/*
 * Copyright (c) 2023 Jan Gnip
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-3-CLAUSE
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <drivers/scd30.h>
#include <zephyr/sys/crc.h>

#define DT_DRV_COMPAT sensirion_scd30

#include "scd30_priv.h"

// Number of instances of device
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "SCD30 driver enabled without any devices"
#endif

LOG_MODULE_REGISTER(SCD30, CONFIG_SENSOR_LOG_LEVEL);

static void scd30_data_ready_callback(const struct device *dev);

static void scd30_lock(const struct device *dev)
{
	struct scd30_data *data = dev->data;

	(void)k_sem_take(&data->lock, K_FOREVER);
}

static void scd30_unlock(const struct device *dev)
{
	struct scd30_data *data = dev->data;

	k_sem_give(&data->lock);
}

// Writes the desired command (cmd) into desired I2C bus
// Returns 0 if sucessful or -EIO (General input/output error)
static int scd30_write_command(const struct device *dev, uint16_t cmd)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t tx_buf[2];

	sys_put_be16(cmd, tx_buf);
	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

// Computes CRC (Cyclic Redundancy Check) value
// Return the computed CRC value
static uint8_t scd30_compute_crc(uint8_t *data, uint8_t data_len)
{
	return crc8(data, data_len, SCD30_CRC8_POLYNOMIAL, SCD30_CRC8_INIT, false);
}

// Compares CRC value with actual value
// Returns 0 if sucessful or -EIO (General input/output error)
static int scd30_check_crc(uint8_t *data, uint8_t data_len, uint8_t checksum)
{
	uint8_t actual_crc = scd30_compute_crc(data, data_len);

	if (checksum != actual_crc)
	{
		LOG_ERR("CRC check failed. Expected: %x, got %x", checksum, actual_crc);
		return -EIO;
	}

	return 0;
}

// Writes the whole data (Command, value, CRC) into desired I2C bus
// Returns 0 if sucessful or -EIO (General input/output error)
static int scd30_write_register(const struct device *dev, uint16_t cmd, uint16_t val)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t tx_buf[SCD30_CMD_SINGLE_WORD_BUF_LEN];

	sys_put_be16(cmd, &tx_buf[0]);
	sys_put_be16(val, &tx_buf[2]);
	tx_buf[4] = scd30_compute_crc(&tx_buf[2], sizeof(val));

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

// Reads the data from I2C and returns it into val pointer
// Reg is the register the data must be read (?)
// Returns 0 if sucessful or -EIO (General input/output error)
static int scd30_read_register(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_word rx_word;
	int rc;

	rc = scd30_write_command(dev, reg);
	if (rc != 0)
	{
		return rc;
	}

	rc = i2c_read_dt(&cfg->bus, (uint8_t *)&rx_word, sizeof(rx_word));
	if (rc != 0)
	{
		return rc;
	}

	if (scd30_check_crc(rx_word.word, sizeof(rx_word.word), rx_word.crc) != 0)
	{
		return -EIO;
	}

	*val = sys_get_be16(rx_word.word);
	return 0;
}

/**
 * @brief Take the received  word (16 bit), if the data is valid, add it to dst
 *
 * @param data data received from sensor
 * @param dst buffer to store data
 * @return int
 */
static int scd30_fill_data_buf(struct scd30_word word, uint8_t *dst)
{
	if (scd30_check_crc(word.word, SCD30_WORD_SIZE, word.crc))
	{
		return -EIO;
	}

	dst[0] = word.word[0];
	dst[1] = word.word[1];

	return 0;
}

// Converts the data from byte type to float
// Returns the value stored by the region pointed by bytes in float type
static float scd30_bytes_to_float(const uint8_t *bytes)
{
	union
	{
		uint32_t u32_value;
		float float32;
	} tmp;

	tmp.u32_value = sys_get_be32(bytes);
	return tmp.float32;
}

// Gets the sample time and stores it in dev
// Return 0 if sucessful or rc value
static int scd30_get_sample_time(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	uint16_t sample_time;
	int rc;

	rc = scd30_read_register(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL, &sample_time);
	if (rc != 0)
	{
		return rc;
	}

	data->sample_time = sample_time;

	return 0;
}

// Sets the sample time to dev, if it's not an invalid value
// Returns 0 if sucessful or an error
static int scd30_set_sample_time(const struct device *dev, uint16_t sample_time)
{
	struct scd30_data *data = dev->data;
	int rc;

	if (sample_time < SCD30_MIN_SAMPLE_TIME || sample_time > SCD30_MAX_SAMPLE_TIME)
	{
		return -EINVAL;
	}

	rc = scd30_write_command(dev, SCD30_CMD_STOP_PERIODIC_MEASUREMENT);
	if (rc != 0)
	{
		return rc;
	}

	rc = scd30_write_register(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL, sample_time);
	if (rc != 0)
	{
		return rc;
	}

	data->sample_time = sample_time;

	return scd30_write_register(dev, SCD30_CMD_START_PERIODIC_MEASUREMENT,
								SCD30_MEASUREMENT_DEF_AMBIENT_PRESSURE);
}

static int scd30_data_ready_init(const struct device *dev)
{
    const struct scd30_config *cfg = dev->config;
    int rc;

    // Verifique se o pino `data_ready` está disponível
    if (!device_is_ready(cfg->rdy_gpios.port)) {
        return -ENODEV;
    }

    // Configura a interrupção no pino `data_ready`
    rc = gpio_pin_configure_dt(&cfg->rdy_gpios, GPIO_INPUT | GPIO_INT_EDGE_TO_ACTIVE);
    if (rc != 0) {
        return rc;
    }

    // Registra a função de callback para o pino `data_ready`
    rc = gpio_pin_interrupt_configure_dt(&cfg->rdy_gpios, GPIO_INT_EDGE_TO_ACTIVE);
    if (rc != 0) {
        return rc;
    }

    // Agora, registra o callback para ser chamado quando o evento ocorrer
    gpio_add_callback(cfg->rdy_gpios.port, &scd30_data_ready_callback);

    return 0;
}

// Callback to be called when data from sensor is available
static void scd30_data_ready_callback(const struct device *dev)
{
    struct scd30_data *data = dev->data;
    int rc;

    // Fetching data ...
    rc = scd30_sample_fetch(dev, SENSOR_CHAN_ALL);  
    if (rc != 0) {
        LOG_ERR("Failed to fetch data: %d", rc);
    } 
}

// Fetch the readings of the sensor and stores it into dev
// Returns 0 if sucessful or the corresponding rc error code
static int scd30_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct scd30_data *data = dev->data;
	const struct scd30_config *cfg = dev->config;
	int rc;

	/*
	 * Struct represensting data as received from the SCD30
	 * each scd30_word conists of a 16 bit data word followed
	 * by an 8 bit crc.
	 */
	struct scd30_rx_data
	{
		struct scd30_word co2_msw;
		struct scd30_word co2_lsw;
		struct scd30_word temp_msw;
		struct scd30_word temp_lsw;
		struct scd30_word humidity_msw;
		struct scd30_word humidity_lsw;
	} raw_rx_data;

	/*
	 * Struct representing the received data from the SCD30
	 * in big endian order with the CRC's removed.
	 */
	struct rx_data
	{
		uint8_t co2_be[sizeof(float)];
		uint8_t temp_be[sizeof(float)];
		uint8_t humidity_be[sizeof(float)];
	} rx_data;

	// Checks if selected channel is valid
	if (chan != SENSOR_CHAN_ALL)
	{
		return -ENOTSUP;
	}

	scd30_lock(dev);

	rc = scd30_write_command(dev, SCD30_CMD_READ_MEASUREMENT);
	if (rc != 0)
	{
		LOG_DBG("Failed to send command. (rc = %d)", rc);
		scd30_unlock(dev);
		goto unlock_return;
	}

	rc = i2c_read_dt(&cfg->bus, (uint8_t *)&raw_rx_data, sizeof(raw_rx_data));
	if (rc != 0)
	{
		LOG_DBG("Failed to read data. (rc = %d)", rc);
		goto unlock_return;
	}

	/* C02 data */
	rc = scd30_fill_data_buf(raw_rx_data.co2_msw, &rx_data.co2_be[0]);
	if (rc != 0)
	{
		goto unlock_return;
	}
	rc = scd30_fill_data_buf(raw_rx_data.co2_lsw, &rx_data.co2_be[SCD30_WORD_SIZE]);
	if (rc != 0)
	{
		goto unlock_return;
	}

	/* Temperature data */
	rc = scd30_fill_data_buf(raw_rx_data.temp_msw, &rx_data.temp_be[0]);
	if (rc != 0)
	{
		goto unlock_return;
	}
	rc = scd30_fill_data_buf(raw_rx_data.temp_lsw, &rx_data.temp_be[SCD30_WORD_SIZE]);
	if (rc != 0)
	{
		goto unlock_return;
	}

	/* Relative humidity */
	rc = scd30_fill_data_buf(raw_rx_data.humidity_msw, &rx_data.humidity_be[0]);
	if (rc != 0)
	{
		goto unlock_return;
	}
	rc = scd30_fill_data_buf(raw_rx_data.humidity_lsw, &rx_data.humidity_be[SCD30_WORD_SIZE]);
	if (rc != 0)
	{
		goto unlock_return;
	}

	data->co2_ppm = scd30_bytes_to_float(rx_data.co2_be);
	data->temp = scd30_bytes_to_float(rx_data.temp_be);
	data->rel_hum = scd30_bytes_to_float(rx_data.humidity_be);

	goto unlock_return;

// Libera o bloqueio e retorna
unlock_return:
	scd30_unlock(dev);
	return rc;
}

/*
	 * Struct represensting data as received from the SCD30
	 * each scd30_word conists of a 16 bit data word followed
	 * by an 8 bit crc.
	 */
	// struct scd30_rx_data
	// {
	// 	struct scd30_word co2_msw;
	// 	struct scd30_word co2_lsw;
	// 	struct scd30_word temp_msw;
	// 	struct scd30_word temp_lsw;
	// 	struct scd30_word humidity_msw;
	// 	struct scd30_word humidity_lsw;
	// } raw_rx_data;

	// /*
	//  * Struct representing the received data from the SCD30
	//  * in big endian order with the CRC's removed.
	//  */
	// struct rx_data
	// {
	// 	uint8_t co2_be[sizeof(float)];
	// 	uint8_t temp_be[sizeof(float)];
	// 	uint8_t humidity_be[sizeof(float)];
	// } rx_data;

	// // Espera pino data ready ser 1
	// while (!data_ready) {
	// 	data_ready = gpio_pin_get_dt(&cfg->rdy_gpios);
	// }	

	// while (!data_ready)
	// {
	// 	rc = scd30_read_register(dev, SCD30_CMD_GET_DATA_READY, &data_ready);
	// 	if (rc != 0)
	// 	{
	// 		return rc;
	// 	}
	// 	k_sleep(K_MSEC(3));
	// }

	// if (!data_ready) {
	// 	return -ENODATA;
	// }

	// /* delay for 3 msec as per datasheet. */
	// k_sleep(K_MSEC(3));

	// rc = i2c_read_dt(&cfg->bus, (uint8_t *)&raw_rx_data, sizeof(raw_rx_data));
	// if (rc != 0)
	// {
	// 	LOG_DBG("Failed to read data. (rc = %d)", rc);
	// 	return rc;
	// }

	// /* C02 data */
	// rc = scd30_fill_data_buf(raw_rx_data.co2_msw, &rx_data.co2_be[0]);
	// if (rc != 0)
	// {
	// 	return rc;
	// }
	// rc = scd30_fill_data_buf(raw_rx_data.co2_lsw, &rx_data.co2_be[SCD30_WORD_SIZE]);
	// if (rc != 0)
	// {
	// 	return rc;
	// }

	// /* Temperature data */
	// rc = scd30_fill_data_buf(raw_rx_data.temp_msw, &rx_data.temp_be[0]);
	// if (rc != 0)
	// {
	// 	return rc;
	// }
	// rc = scd30_fill_data_buf(raw_rx_data.temp_lsw, &rx_data.temp_be[SCD30_WORD_SIZE]);
	// if (rc != 0)
	// {
	// 	return rc;
	// }

	// /* Relative humidity */
	// rc = scd30_fill_data_buf(raw_rx_data.humidity_msw, &rx_data.humidity_be[0]);
	// if (rc != 0)
	// {
	// 	return rc;
	// }
	// rc = scd30_fill_data_buf(raw_rx_data.humidity_lsw, &rx_data.humidity_be[SCD30_WORD_SIZE]);
	// if (rc != 0)
	// {
	// 	return rc;
	// }

	// data->co2_ppm = scd30_bytes_to_float(rx_data.co2_be);
	// data->temp = scd30_bytes_to_float(rx_data.temp_be);
	// data->rel_hum = scd30_bytes_to_float(rx_data.humidity_be);


// Stores the desired value into dev data. The value is chosen by the chan parameter
// Returns 0 if sucessful or -1 if the channel isn't correct
static int scd30_channel_get(const struct device *dev, enum sensor_channel chan,
							 struct sensor_value *val)
{
	struct scd30_data *data = dev->data;

	switch (chan)
	{
	case SENSOR_CHAN_CO2:
		sensor_value_from_double(val, data->co2_ppm);
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		sensor_value_from_double(val, data->temp);
		break;

	case SENSOR_CHAN_HUMIDITY:
		sensor_value_from_double(val, data->rel_hum);
		break;

	default:
		return -1;
	}
	return 0;
}

// Stores the desided attribute into val, accordingly to attr parameter
// Returns 0 if sucessful or -ENOTSUP if the channel or attribute is incorrect
static int scd30_attr_get(const struct device *dev, enum sensor_channel chan,
						  enum sensor_attribute attr, struct sensor_value *val)
{
	struct scd30_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL)
	{
		return -ENOTSUP;
	}

	switch (attr)
	{
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	{
		float frequency = 1.0 / data->sample_time;

		sensor_value_from_double(val, frequency);
		return 0;
	}
	case SCD30_SENSOR_ATTR_SAMPLING_PERIOD:
	{
		val->val1 = data->sample_time;
		val->val2 = 0;
		return 0;
	}

	default:
		return -ENOTSUP;
	}
}

// Sets the desired attribute into dev, accordingly with attr parameter
// Returns 0 if sucessful, -ENOTSUP if channel or attr is incorrect or error at setting the attribute
static int scd30_attr_set(const struct device *dev, enum sensor_channel chan,
						  enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL)
	{
		return -ENOTSUP;
	}

	switch (attr)
	{
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
	{
		double frequency = sensor_value_to_double(val);
		double sample_time = 1.0 / frequency;

		return scd30_set_sample_time(dev, (uint16_t)sample_time);
	}
	case SCD30_SENSOR_ATTR_SAMPLING_PERIOD:
	{
		return scd30_set_sample_time(dev, (uint16_t)val->val1);
	}
	default:
		return -ENOTSUP;
	}
}

// Struct used to integrate this driver functions into Zephyr sensor's API
static const struct sensor_driver_api scd30_driver_api = {
	.sample_fetch = scd30_sample_fetch,
	.channel_get = scd30_channel_get,
	.attr_get = scd30_attr_get,
	.attr_set = scd30_attr_set,
};

// Initialize the SCD30 sensor
static int scd30_init(const struct device *dev)
{
	LOG_DBG("Initializing SCD30");
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	int rc = 0;

	if (!device_is_ready(cfg->bus.bus))
	{
		LOG_ERR("Failed to get pointer to %s device!", cfg->bus.bus->name);
		return -ENODEV;
	}

	// Initialize and configures the interrup pin and its callback
	rc = scd30_data_ready_init(dev);
	if (rc != 0)
	{
		LOG_ERR("Failed to initialize gpio pin!");
	}

	// really ugly workaround to make I2C1 work at 50KHz
	((NRF_TWIM_Type *)NRF_TWIM1_BASE)->FREQUENCY = 0x00550000UL;

	rc = scd30_set_sample_time(dev, data->sample_time);
	if (rc != 0)
	{
		LOG_WRN("Failed to set sample period. Using period stored of device");
		/* Try to read sample time from sensor to reflect actual sample period */
		rc = scd30_get_sample_time(dev);
	}

	LOG_DBG("Sample time: %d", data->sample_time);
	if (rc != 0)
	{
		return rc;
	}

	LOG_DBG("Starting periodic measurements");
	rc = scd30_write_register(dev, SCD30_CMD_START_PERIODIC_MEASUREMENT,
							  SCD30_MEASUREMENT_DEF_AMBIENT_PRESSURE);

	return rc;
}

#define SCD30_DEFINE(inst)                                                                  \
	static struct scd30_data scd30_data_##inst = {                                          \
		.sample_time = DT_INST_PROP(inst, sample_period)};                                  \
	static const struct scd30_config scd30_config_##inst = {                                \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                  \
		.rdy_gpios = GPIO_DT_SPEC_INST_GET(inst, rdy_gpios),                       			\
	};                                                                                      \
                                                                                            \
	DEVICE_DT_INST_DEFINE(inst, scd30_init, NULL, &scd30_data_##inst, &scd30_config_##inst, \
						  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &scd30_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCD30_DEFINE);