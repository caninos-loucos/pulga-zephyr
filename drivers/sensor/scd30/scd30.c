/*
 * Copyright (c) 2025 Matheus de Almeida Orsi e Silva
 * Copyright (c) 2024 Kauê Rodrigues Barbosa
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
#include "scd30_priv.h"

#define DT_DRV_COMPAT sensirion_scd30

// Number of instances of device
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "SCD30 driver enabled without any devices"
#endif

LOG_MODULE_REGISTER(SCD30, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Fetch a sample from the SCD30 sensor.
 *
 * This function reads the CO2, temperature, and humidity data from the SCD30 sensor.
 * It first checks if the sensor data is ready to be read, then reads the raw data
 * from the sensor, processes it, and stores the results in the device data structure.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan The sensor channel to read. Only SENSOR_CHAN_ALL is supported.
 *
 * @return 0 if successful, negative error code otherwise.
 */
static int scd30_sample_fetch(const struct device *dev, enum sensor_channel chan);

/**
 * @brief Writes a command to the SCD30 sensor.
 *
 * This function sends a command to the SCD30 sensor over the I2C bus.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cmd The command to be sent to the sensor.
 * @return 0 if successful, or a negative error code on failure.
 */
static int scd30_write_command(const struct device *dev, uint16_t cmd)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t tx_buf[2];

	// Convert the command to big-endian format and store it in the transmit buffer
	sys_put_be16(cmd, tx_buf);

	// Write the command to the I2C bus
	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

/**
 * @brief Compute the CRC for the given data.
 *
 * This function calculates an 8-bit CRC for the provided data using the
 * SCD30 CRC8 polynomial and initial value.
 *
 * @param data Pointer to the data array for which the CRC is to be computed.
 * @param data_len Length of the data array.
 *
 * @return The computed 8-bit CRC value.
 */
static uint8_t scd30_compute_crc(uint8_t *data, uint8_t data_len)
{
	return crc8(data, data_len, SCD30_CRC8_POLYNOMIAL, SCD30_CRC8_INIT, false);
}

/**
 * @brief Check the CRC of the given data.
 *
 * This function calculates the CRC for the provided data and compares it
 * with the given checksum to verify data integrity.
 *
 * @param data Pointer to the data array for which the CRC is to be checked.
 * @param data_len Length of the data array.
 * @param checksum The expected CRC value to compare against.
 *
 * @return 0 if the CRC matches, or -EIO if the CRC check fails.
 */
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

/**
 * @brief Write a value to a specific register of the SCD30 sensor.
 *
 * This function sends a command and a value to a specified register of the SCD30 sensor
 * using the I2C bus. The command and value are converted to big-endian format and a CRC
 * is computed for the value.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cmd The command to be sent to the sensor.
 * @param val The value to be written to the register.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
static int scd30_write_register(const struct device *dev, uint16_t cmd, uint16_t val)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t tx_buf[SCD30_CMD_SINGLE_WORD_BUF_LEN];

	sys_put_be16(cmd, &tx_buf[0]);
	sys_put_be16(val, &tx_buf[2]);
	tx_buf[4] = scd30_compute_crc(&tx_buf[2], sizeof(val));

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

/**
 * @brief Read a value from a specific register of the SCD30 sensor.
 *
 * This function reads a value from a specified register of the SCD30 sensor
 * using the I2C bus. It sends a command to the sensor to read the register,
 * then reads the response and checks the CRC for data integrity.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg The register address to read from the sensor.
 * @param val Pointer to store the read value.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
static int scd30_read_register(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_word rx_word;
	int rc;

	// Send command to read the register
	rc = scd30_write_command(dev, reg);
	if (rc != 0)
	{
		return rc;
	}

	k_sleep(K_MSEC(3)); // Wait for the sensor to process the command

	// Read the response from the sensor
	rc = i2c_read_dt(&cfg->bus, (uint8_t *)&rx_word, sizeof(rx_word));
	if (rc != 0)
	{
		return rc;
	}

	// Check the CRC of the received data
	if (scd30_check_crc(rx_word.word, sizeof(rx_word.word), rx_word.crc) != 0)
	{
		return -EIO;
	}

	// Convert the received data to host-endian format and store it in val
	*val = sys_get_be16(rx_word.word);
	return 0;
}

/**
 * @brief Validate and store a 16-bit word received from the sensor.
 *
 * This function checks the CRC of the received word. If the CRC is valid,
 * it stores the word in the destination buffer.
 *
 * @param word The 16-bit word received from the sensor.
 * @param dst The buffer to store the valid data.
 * @return 0 if successful, or -EIO if the CRC check fails.
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

/**
 * @brief Convert byte array to float.
 *
 * This function converts a 4-byte array in big-endian format to a float.
 *
 * @param bytes Pointer to the byte array to be converted.
 * @return The float value represented by the byte array.
 */
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

/**
 * @brief Retrieve the sample time from the SCD30 sensor.
 *
 * This function reads the sample time (measurement interval) from the SCD30 sensor
 * and stores it in the device's data structure.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return 0 if successful, or a negative error code on failure.
 */
static int scd30_get_sample_time(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	uint16_t sample_time;
	int rc;

	rc = scd30_read_register(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL, &sample_time);
	if (rc != 0)
	{
		LOG_ERR("Failed to read sample time: %d", rc);
		return rc;
	}

	data->sample_time = sample_time;

	return 0;
}

/**
 * @brief Retrieves the automatic self-calibration status.
 *
 * This function reads the automatic self-calibration status from the SCD30 sensor
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param enabled Pointer to a sensor_value structure where the status will be stored.
 * @return 0 if successful, or a negative error code on failure.
 */
static int scd30_get_auto_calibration(const struct device *dev, struct sensor_value *enabled)
{
	uint16_t auto_self_calibration;
	int rc;

	rc = scd30_read_register(dev, SCD30_CMD_AUTO_SELF_CALIBRATION, &auto_self_calibration);
	if (rc != 0)
	{
		return rc;
	}

	enabled->val1 = (auto_self_calibration != 0) ? 1 : 0;
	enabled->val2 = 0;

	return 0;
}

/**
 * @brief Retrieves the CO2 reference value for calibration.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param co2_reference Pointer to a sensor_value structure where the CO2 reference value will be stored.
 * @return 0 if successful, or a negative error code on failure.
 */
static int scd30_get_co2_reference(const struct device *dev, struct sensor_value *co2_reference)
{
	uint16_t calibration_reference;
	int rc;

	rc = scd30_read_register(dev, SCD30_CMD_SET_FORCED_RECALIBRATION, &calibration_reference);
	if (rc != 0)
	{
		return rc;
	}

	co2_reference->val1 = calibration_reference;
	co2_reference->val2 = 0;

	return 0;
}

/**
 * @brief Retrieves the temperature offset for the SCD30 sensor.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param temperature_offset Pointer to a sensor_value structure where the temperature offset
 * will be stored in degrees Celsius.
 * @return 0 if successful, or a negative error code on failure.
 */
static int scd30_get_temperature_offset(const struct device *dev,
										struct sensor_value *temperature_offset)
{
	uint16_t temperature_offset_raw;
	float temperature_offset_degrees;
	int rc;

	// Read the temperature offset register (degrees Celsius * 100)
	rc = scd30_read_register(dev, SCD30_CMD_SET_TEMPERATURE_OFFSET, &temperature_offset_raw);
	if (rc != 0)
	{
		return rc;
	}

	// Convert the raw temperature offset to degrees Celsius
	temperature_offset_degrees = (float)temperature_offset_raw / 100.0f;
	sensor_value_from_double(temperature_offset, temperature_offset_degrees);

	return 0;
}

/**
 * @brief Sets the sample time for the SCD30 sensor.
 *
 * This function sets the sample time for the SCD30 sensor, ensuring that the
 * provided sample time is within the valid range defined by SCD30_MIN_SAMPLE_TIME
 * and SCD30_MAX_SAMPLE_TIME. If the sample time is valid and different from the
 * one configured in the chip, the driver updates the measurement interval.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param sample_time Desired sample time in seconds.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
static int scd30_set_sample_time(const struct device *dev, uint16_t sample_time)
{
	struct scd30_data *data = dev->data;
	int rc;

	if (sample_time < SCD30_MIN_SAMPLE_TIME || sample_time > SCD30_MAX_SAMPLE_TIME)
	{
		return -EINVAL;
	}

	// Avoid writing in the register if the sample time is already set because it
	// can interfere with the sensor accuracy. Changing the sample time requires
	// the recalibration of the sensor.
	rc = scd30_get_sample_time(dev);
	if (rc != 0)
	{
		return rc;
	}
	if (data->sample_time == sample_time)
	{
		LOG_DBG("Sample time already set to %d seconds", sample_time);
		return rc;
	}

	rc = scd30_write_register(dev, SCD30_CMD_SET_MEASUREMENT_INTERVAL, sample_time);
	if (rc != 0)
	{
		return rc;
	}
	data->sample_time = sample_time;

	return 0;
}

/**
 * @brief Get the sensor value for a specific channel.
 *
 * This function retrieves the sensor value for the specified channel
 * from the SCD30 sensor device and saves it to dev.data.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan The sensor channel to get the value from. Supported channels:
 *             - SENSOR_CHAN_CO2: CO2 concentration in parts per million (ppm).
 *             - SENSOR_CHAN_AMBIENT_TEMP: Ambient temperature in degrees Celsius.
 *             - SENSOR_CHAN_HUMIDITY: Relative humidity in percentage.
 * @param val Pointer to the sensor_value structure where the value is stored.
 *
 * @return 0 on success, -1 if the specified channel is not supported.
 */
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

/**
 * @brief Get the attribute value for the SCD30 sensor.
 *
 * This function retrieves the value of the specified attribute for the SCD30 sensor.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan The sensor channel to get the attribute from. Only SENSOR_CHAN_ALL is supported.
 * @param attr The sensor attribute to get the value of. Supported attributes:
 *             - SENSOR_ATTR_SAMPLING_FREQUENCY: Sampling frequency in Hz.
 *             - SCD30_SENSOR_ATTR_SAMPLING_PERIOD: Sampling period in seconds.
 * 			   - SCD30_SENSOR_ATTR_AUTO_SELF_CALIBRATION: Enable/disable auto self-calibration.
 * 		   	   - SCD30_SENSOR_ATTR_FORCED_RECALIBRATION: Set CO2 reference value for calibration.
 * 			   - SCD30_SENSOR_ATTR_TEMPERATURE_OFFSET: Set temperature offset in degrees Celsius.
 * @param val Pointer to the sensor_value structure where the retrieved value will be stored.
 *
 * @return 0 on success, -ENOTSUP if the specified channel or attribute is not supported.
 */
static int scd30_attr_get(const struct device *dev, enum sensor_channel chan,
						  enum sensor_attribute attr, struct sensor_value *val)
{
	struct scd30_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL)
	{
		return -ENOTSUP;
	}

	switch ((uint16_t)attr)
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
	case SCD30_SENSOR_ATTR_AUTO_SELF_CALIBRATION:
	{
		return scd30_get_auto_calibration(dev, val);
	}
	case SCD30_SENSOR_ATTR_FORCED_RECALIBRATION:
	{
		return scd30_get_co2_reference(dev, val);
	}
	case SCD30_SENSOR_ATTR_TEMPERATURE_OFFSET:
	{
		return scd30_get_temperature_offset(dev, val);
	}
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Set the attribute value for the SCD30 sensor.
 *
 * This function sets the value of the specified attribute for the SCD30 sensor.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan The sensor channel to set the attribute for. Only SENSOR_CHAN_ALL is supported.
 * @param attr The sensor attribute to set the value of. Supported attributes:
 *             - SENSOR_ATTR_SAMPLING_FREQUENCY: Sampling frequency in Hz.
 *             - SCD30_SENSOR_ATTR_SAMPLING_PERIOD: Sampling period in seconds.
 * 			   - SCD30_SENSOR_ATTR_AUTO_SELF_CALIBRATION: Enable/disable auto self-calibration.
 * 		   	   - SCD30_SENSOR_ATTR_FORCED_RECALIBRATION: Set CO2 reference value for calibration.
 * 			   - SCD30_SENSOR_ATTR_TEMPERATURE_OFFSET: Set temperature offset in degrees Celsius.
 * 			   - SCD30_SENSOR_ATTR_PRESSURE: Set pressure offset in mBar (kPa * 10).
 * @param val Pointer to the sensor_value structure containing the value to be set.
 *
 * @return 0 on success, -ENOTSUP if the specified channel or attribute is not supported.
 */
static int scd30_attr_set(const struct device *dev, enum sensor_channel chan,
						  enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL)
	{
		return -ENOTSUP;
	}

	switch ((uint16_t)attr)
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
	case SCD30_SENSOR_ATTR_AUTO_SELF_CALIBRATION:
	{
		uint16_t auto_self_calibration = (val->val1 == 0) ? 0 : 1;
		return scd30_write_register(dev, SCD30_CMD_AUTO_SELF_CALIBRATION, auto_self_calibration);
	}
	case SCD30_SENSOR_ATTR_FORCED_RECALIBRATION:
	{
		uint16_t co2_reference = val->val1;
		if (co2_reference < SCD30_MIN_CO2_REFERENCE || co2_reference > SCD30_MAX_CO2_REFERENCE)
		{
			return -EINVAL;
		}
		return scd30_write_register(dev, SCD30_CMD_SET_FORCED_RECALIBRATION, co2_reference);
	}
	case SCD30_SENSOR_ATTR_TEMPERATURE_OFFSET:
	{	
		// Guarantee that the temperature offset is non-negative
		uint16_t temperature_offset = (uint16_t)(MAX(0, (sensor_value_to_float(val) * 100.0f)));
		return scd30_write_register(dev, SCD30_CMD_SET_TEMPERATURE_OFFSET, temperature_offset);
	}
	case SCD30_SENSOR_ATTR_PRESSURE:
	{
		uint16_t pressure_offset = val->val1;
		if (pressure_offset != 0 && (pressure_offset < SCD30_MIN_PRESSURE_OFFSET ||
									 pressure_offset > SCD30_MAX_PRESSURE_OFFSET))
		{
			return -EINVAL;
		}
		return scd30_write_register(dev, SCD30_CMD_START_PERIODIC_MEASUREMENT, pressure_offset);
	}
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Data ready handler for the SCD30 sensor.
 *
 * This function is called when the data ready interrupt is triggered.
 * It submits a work item to process the data.
 *
 * @param dev Pointer to the device structure that represents the gpio that caused the interruption.
 * @param cb Pointer to the GPIO callback structure.
 * @param pins The pin mask for the GPIO interrupt.
 */
static void scd30_data_ready_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(pins);

	struct scd30_data *data = CONTAINER_OF(cb, struct scd30_data, callback_data_ready);
	k_work_submit(&data->data_ready_work); // Triggers work schedule to be executed in due time
}

/**
 * @brief Trigger application callback function.
 *
 * This function is called to perform a sensor read operation and calls the
 * registered application callback if it is set.
 * If the read operation fails, an error message is logged.
 *
 * @param work Pointer to the work structure (unused).
 */
void trigger_application_callback(struct k_work *work)
{
	ARG_UNUSED(work);
	int rc = 0;

	struct scd30_data *data = CONTAINER_OF(work, struct scd30_data, data_ready_work);
	rc = scd30_sample_fetch(data->dev, SENSOR_CHAN_ALL);
	if (rc != 0)
	{
		LOG_ERR("Error at reading");
		return;
	}

	// Call application callback if registered
	if (data->registered_callback)
	{
		data->registered_callback();
	}
}

/**
 * @brief Register a callback function for the SCD30 sensor.
 *
 * This function registers a callback function that will be called when the
 * SCD30 sensor data is ready. This callback is defined in the application.
 * It initializes the work item and configures the GPIO pin for data ready interrupt.
 *
 * @param cb The callback function to register.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval None
 */
void scd30_register_callback(const struct device *dev, scd30_callback_t cb)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	int rc = 0;

	data->registered_callback = cb;
	k_work_init(&data->data_ready_work, trigger_application_callback);

	if (!gpio_is_ready_dt(&cfg->rdy_gpios))
	{
		LOG_ERR("Error: ready_pin device %s is not ready\n",
				cfg->rdy_gpios.port->name);
		return;
	}

	rc = gpio_pin_configure_dt(&cfg->rdy_gpios, GPIO_INPUT);
	if (rc != 0)
	{
		LOG_ERR("%s: failed to initialize GPIO for data ready", dev->name);
		return;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->rdy_gpios, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc != 0)
	{
		LOG_ERR("%s: failed to configure data ready interrupt", dev->name);
		return;
	}

	gpio_init_callback(&data->callback_data_ready, scd30_data_ready_handler, BIT(cfg->rdy_gpios.pin));
	rc = gpio_add_callback(cfg->rdy_gpios.port, &data->callback_data_ready);
	if (rc != 0)
	{
		LOG_ERR("%s: failed to add data ready callback", dev->name);
		return;
	}

	// This forces the first reading. For some reason, it is needed to signalize a
	// change in the data ready signal (that comes from the physical pin). If not done,
	// the interrupt do not work properly.
	rc = scd30_sample_fetch(dev, SENSOR_CHAN_ALL);
}

static int scd30_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint16_t data_ready;
	struct scd30_data *data = dev->data;
	const struct scd30_config *cfg = dev->config;
	int rc = 0;

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
	 * struct representing the received data from the SCD30
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

	rc = scd30_read_register(dev, SCD30_CMD_GET_DATA_READY, &data_ready);
	if (rc != 0)
	{
		goto return_clause;
	}
	while (!data_ready)
	{
		rc = scd30_read_register(dev, SCD30_CMD_GET_DATA_READY, &data_ready);

		if (rc != 0)
		{
			goto return_clause;
		}
	}
	LOG_DBG("SCD30 data ready");

	rc = scd30_write_command(dev, SCD30_CMD_READ_MEASUREMENT);
	if (rc != 0)
	{
		LOG_DBG("Failed to send command. (rc = %d)", rc);
		goto return_clause;
	}

	/* delay for 3 msec as per datasheet. */
	k_sleep(K_MSEC(3));

	rc = i2c_read_dt(&cfg->bus, (uint8_t *)&raw_rx_data, sizeof(raw_rx_data));
	if (rc != 0)
	{
		LOG_DBG("Failed to read data. (rc = %d)", rc);
		goto return_clause;
	}

	/* C02 data */
	rc = scd30_fill_data_buf(raw_rx_data.co2_msw, &rx_data.co2_be[0]);
	if (rc != 0)
	{
		goto return_clause;
	}
	rc = scd30_fill_data_buf(raw_rx_data.co2_lsw, &rx_data.co2_be[SCD30_WORD_SIZE]);
	if (rc != 0)
	{
		goto return_clause;
	}

	/* temperature data */
	rc = scd30_fill_data_buf(raw_rx_data.temp_msw, &rx_data.temp_be[0]);
	if (rc != 0)
	{
		goto return_clause;
	}
	rc = scd30_fill_data_buf(raw_rx_data.temp_lsw, &rx_data.temp_be[SCD30_WORD_SIZE]);
	if (rc != 0)
	{
		goto return_clause;
	}

	/* relative humidity */
	rc = scd30_fill_data_buf(raw_rx_data.humidity_msw, &rx_data.humidity_be[0]);
	if (rc != 0)
	{
		goto return_clause;
	}
	rc = scd30_fill_data_buf(raw_rx_data.humidity_lsw, &rx_data.humidity_be[SCD30_WORD_SIZE]);
	if (rc != 0)
	{
		goto return_clause;
	}

	data->co2_ppm = scd30_bytes_to_float(rx_data.co2_be);
	data->temp = scd30_bytes_to_float(rx_data.temp_be);
	data->rel_hum = scd30_bytes_to_float(rx_data.humidity_be);

return_clause:
	if (rc != 0)
	{
		LOG_ERR("%s: reading sample failed. (rc = %d)", dev->name, rc);
	}
	return rc;
}

/**
 * @brief Start periodic measurements on the SCD30 sensor.
 *
 * This function sends a command to the SCD30 sensor to start periodic measurements
 * with the default ambient pressure setting.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ambient_pressure The ambient pressure in mBar (kPa * 10) to be used for adjusting
 * the CO2 measurements. If set to 0, the sensor will not compensate for ambient pressure.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
int scd30_start_periodic_measurement(const struct device *dev, int ambient_pressure)
{
	int rc;

	LOG_DBG("Starting periodic measurements");
	if (ambient_pressure != 0 && (ambient_pressure < SCD30_MIN_PRESSURE_OFFSET ||
								  ambient_pressure > SCD30_MAX_PRESSURE_OFFSET))
	{
		LOG_ERR("Invalid ambient pressure value: %d", ambient_pressure);
		return -EINVAL;
	}
	rc = scd30_write_register(dev, SCD30_CMD_START_PERIODIC_MEASUREMENT, ambient_pressure);
	if (rc != 0)
	{
		LOG_ERR("Failed to start periodic measurement");
		return rc;
	}
	LOG_DBG("Periodic measurement started");
	return 0;
}

/**
 * @brief Stop periodic measurements on the SCD30 sensor.
 *
 * This function sends a command to the SCD30 sensor to stop periodic measurements.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 if successful, or a negative error code on failure.
 */
int scd30_stop_periodic_measurement(const struct device *dev)
{
	int rc;

	LOG_DBG("Stopping periodic measurements");
	rc = scd30_write_command(dev, SCD30_CMD_STOP_PERIODIC_MEASUREMENT);
	if (rc != 0)
	{
		LOG_ERR("Failed to stop periodic measurement");
		return rc;
	}
	LOG_DBG("Periodic measurement stopped");
	return 0;
}

// Sensor driver API structure for the SCD30 sensor
static const struct sensor_driver_api scd30_driver_api = {
	.sample_fetch = scd30_sample_fetch,
	.channel_get = scd30_channel_get,
	.attr_get = scd30_attr_get,
	.attr_set = scd30_attr_set,
};

// Initialize the SCD30 sensor and start periodic measurements.
static int scd30_init(const struct device *dev)
{
	LOG_DBG("Initializing SCD30");
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	int rc = 0;

	data->dev = dev;

	k_sem_init(&data->lock, 0, 1);

	if (!device_is_ready(cfg->bus.bus))
	{
		LOG_ERR("Failed to get pointer to %s device!", cfg->bus.bus->name);
		return -ENODEV;
	}

	// Really ugly workaround to make I2C1 work at 50KHz
	((NRF_TWIM_Type *)NRF_TWIM1_BASE)->FREQUENCY = 0x00500000UL;

	rc = scd30_get_sample_time(dev);
	if (rc != 0)
	{
		return rc;
	}
	LOG_DBG("Sample time: %d", data->sample_time);
	LOG_DBG("Sensor initialized, periodic measurements may be started");

	return 0;
}

#define SCD30_DEFINE(inst)                                                                  \
	static struct scd30_data scd30_data_##inst = {};                                        \
	static const struct scd30_config scd30_config_##inst = {                                \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                  \
		.rdy_gpios = GPIO_DT_SPEC_INST_GET(inst, rdy_gpios),                                \
	};                                                                                      \
                                                                                            \
	DEVICE_DT_INST_DEFINE(inst, scd30_init, NULL, &scd30_data_##inst, &scd30_config_##inst, \
						  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &scd30_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCD30_DEFINE);