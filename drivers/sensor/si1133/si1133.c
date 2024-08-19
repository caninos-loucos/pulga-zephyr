/*
 * Copyright (c) 2024 Edgar Bernardi Righi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <drivers/si1133.h>

/**
 * If the device is compatible with "vendor,device" in the
 * board devicetree or in the app overlay, here in the driver
 * it should be compatible with "vendor_device"
 */
#define DT_DRV_COMPAT silabs_si1133

#include "si1133_priv.h"

// Log level comes from application
LOG_MODULE_REGISTER(SI1133, CONFIG_SENSOR_LOG_LEVEL);

// Number of instances of device
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "SI1133 driver enabled without any devices"
#endif

// Clears the command counter 
static int si1133_cmd_counter_clear(const struct device *dev)
{
	struct si1133_data *data = dev->data;
	uint8_t rsp0; // RESPONSE0 register
	int ret; 
		
	// Writes to register
	if ((ret = si1133_cmd_write(dev, SI1133_CMD_REG_RST_CMD_CTR)) < 0) {
		LOG_DBG("cmd ctr clear: reg write failed");
		return ret;
	}
	if ((ret = si1133_rsp0_read(dev, &rsp0)) < 0) {
		LOG_DBG("rsp0 read failed");
		return ret;
	}
	// Stores last 4 bits of RESPONSE0
	data->cmd_counter = (rsp0 & SI1133_RESPONSE0_MSK_CMD_CTR);
	return 0;
}

// Increment command counter after a successful command
static int si1133_cmd_counter_wait_increment(const struct device *dev)
{
	struct si1133_data *data = dev->data;
	uint8_t next = data->cmd_counter + 1; // Next value for counter
	int ret, retry;
	uint8_t rsp0; // RESPONSE0 register
	
	// Repeatedly waits and tries to read rsp0
	for (retry = 0; retry < SI1133_VAL_RETRY; retry++)
	{
		k_sleep(K_MSEC(SI1133_VAL_DELAY_MS));
		
		if ((ret = si1133_rsp0_read(dev, &rsp0)) < 0) {
			LOG_DBG("rsp0 read failed");
			return ret;
		}
		// If error bit is set, last 4 bits of RESPONSE0 are the error code
		if (rsp0 & SI1133_RESPONSE0_BIT_CMD_ERR) {
			LOG_DBG("cmd failed: 0x%x", rsp0 & SI1133_RESPONSE0_MSK_CMD_CTR);
			(void)si1133_cmd_counter_clear(dev);
			return -EIO;
		}
		// Verifies command counter was incremented
		if ((rsp0 & SI1133_RESPONSE0_MSK_CMD_CTR) == next) {
			data->cmd_counter = next;
			return 0;
		}
	}
	LOG_DBG("wait ctr increment timedout");
	return -ETIMEDOUT;
}

static int si1133_param_query(const struct device *dev,
                              uint8_t addr, uint8_t *val)
{
	int ret;
	
	addr |= SI1133_CMD_REG_PRM_QRY_PRFX;
	
	if ((ret = si1133_cmd_counter_clear(dev)) < 0) {
		return ret;
	}
	if ((ret = si1133_cmd_write(dev, addr)) < 0) {
		return ret;
	}
	if ((ret = si1133_cmd_counter_wait_increment(dev)) < 0) {
		return ret;
	}
	return si1133_rsp1_read(dev, val);
}

static int si1133_param_set(const struct device *dev,
                            uint8_t addr, uint8_t val)
{
	uint8_t rsp1;
	int ret;
	
	addr |= SI1133_CMD_REG_PRM_SET_PRFX;
	
	if ((ret = si1133_cmd_counter_clear(dev)) < 0) {
		return ret;
	}
	if ((ret = si1133_hostin0_write(dev, addr, val)) < 0) {
		return ret;
	}
	if ((ret = si1133_cmd_counter_wait_increment(dev)) < 0) {
		return ret;
	}
	if ((ret = si1133_rsp1_read(dev, &rsp1)) < 0) {
		return ret;
	}
	
	return (rsp1 != val) ? -EIO : 0;
}

static int si1133_param_update(const struct device *dev,
                               uint8_t addr, uint8_t msk, uint8_t val)
{
	uint8_t orig, next;
	int ret;
	
	if ((ret = si1133_param_query(dev, addr, &orig)) < 0) {
		return ret;
	}
	next = (orig & ~msk) | (val & msk);
	
	if (next != orig) {
		return si1133_param_set(dev, addr, next);
	}
	return 0;
}

static int si1133_set_bl_mode(const struct device *dev, uint8_t enable)
{
	uint8_t msk = SI1133_CFG_ADCPOSTX_24BIT_OUT;
	uint8_t val = enable ? msk : 0x0;
	int ret;
	
	ret = si1133_param_update(dev, SI1133_PRM_TBL_ADCPOST0, msk, val);
	if (ret < 0) {
		return ret;
	}
	ret = si1133_param_update(dev, SI1133_PRM_TBL_ADCPOST1, msk, val);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

static int si1133_start_meas(const struct device *dev)
{
	int ret;
	
	if ((ret = si1133_cmd_counter_clear(dev)) < 0) {
		return ret;
	}
	if ((ret = si1133_cmd_write(dev, SI1133_CMD_REG_FORCE)) < 0) {
		return ret;
	}
	if ((ret = si1133_cmd_counter_wait_increment(dev)) < 0) {
		return ret;
	}
	return 0;
}

static int si1133_wait_meas(const struct device *dev)
{
	const uint8_t irq_status = SI1133_IRQ_CHANNEL_0 | SI1133_IRQ_CHANNEL_1 | 
	                           SI1133_IRQ_CHANNEL_2;
	uint8_t status;
	int ret, retry;
	
	for (retry = 0; retry < SI1133_VAL_RETRY; retry++)
	{
		k_sleep(K_MSEC(SI1133_VAL_DELAY_MS));
		
		if ((ret = si1133_irq_read(dev, &status)) < 0) {
			return ret;
		}
		if ((status & irq_status) == irq_status) {
			return 0;
		}
	}
	return -ETIMEDOUT;
}

static inline int32_t si1133_calculate_uvi(int32_t uv) {
	int64_t x = (int64_t)uv;
	int64_t y = ((INT64_C(391) * x * x) / INT64_C(100)) + (x * INT64_C(1000));
	return (int32_t)((INT64_C(187) * y) / INT64_C(100000));
}

static inline int32_t si1133_24bit_sign_extend(uint32_t value) {
	return (((int32_t)(value << 8)) / 256); 
}

static int si1133_fetch_meas(const struct device *dev)
{
	uint8_t size, buf[SI1133_CFG_TOTAL_OUTPUT_BYTES_MAX];
	struct si1133_data *data = dev->data;
	int ret;
	
	if (data->bl_mode_enabled) {
		size = SI1133_CFG_TOTAL_OUTPUT_BYTES_BL;
	}
	else {
		size = SI1133_CFG_TOTAL_OUTPUT_BYTES_LL;
	}
	
	ret = si1133_reg_read(dev, SI1133_I2C_REG_HOSTOUT_BASE, buf, size);
	if (ret < 0) {
		return ret;
	}
	
	if (data->bl_mode_enabled) { /* 24-bit signed integer (light and ir) */
		uint32_t chan_light = (buf[0] << 16) | (buf[1] << 8) | buf[2];
		uint32_t chan_ir = (buf[3] << 16) | (buf[4] << 8) | buf[5];
		data->chan_light = si1133_24bit_sign_extend(chan_light);
		data->chan_ir = si1133_24bit_sign_extend(chan_ir);
		data->chan_uv = (uint16_t)((buf[6] << 8) | buf[7]);
	}
	else { /* 16-bit unsigned integer (all channels) */
		data->chan_light = (uint16_t)((buf[0] << 8) | buf[1]);
		data->chan_ir = (uint16_t)((buf[2] << 8) | buf[3]);
		data->chan_uv = (uint16_t)((buf[4] << 8) | buf[5]);
	}
	/* Calculate UV index */
	data->chan_uvi = si1133_calculate_uvi(data->chan_uv);
	return 0;
}

static int si1133_sample_fetch(const struct device *dev,
                               enum sensor_channel chan)
{
	struct si1133_data *data = dev->data;
	int ret;
	
	ARG_UNUSED(chan);
	
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;
	(void)pm_device_state_get(dev, &state);
	/* Do not allow sample fetching from suspended state */
	if (state == PM_DEVICE_STATE_SUSPENDED)
		return -EIO;
#endif
	
	if ((ret = si1133_set_bl_mode(dev, data->bl_mode_enabled)) < 0) {
		return ret;
	}
	if ((ret = si1133_start_meas(dev)) < 0) {
		return ret;
	}
	if ((ret = si1133_wait_meas(dev)) < 0) {
		return ret;
	}
	return si1133_fetch_meas(dev);
}

static int si1133_channel_get(const struct device *dev,
                              enum sensor_channel chan,
                              struct sensor_value *val)
{
	struct si1133_data *data = dev->data;
	
	switch ((int)(chan)) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = data->chan_light;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_IR:
		val->val1 = data->chan_ir;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_UV:
		val->val1 = data->chan_uv;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_UVI:
		val->val1 = (int32_t)(data->chan_uvi / 100);
		val->val2 = (int32_t)(data->chan_uvi % 100 * 10000);
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int si1133_attr_set(const struct device *dev, enum sensor_channel chan,
                           enum sensor_attribute attr,
                           const struct sensor_value *val)
{
	struct si1133_data *data = dev->data;
	ARG_UNUSED(chan);
	
	if ((int)attr == SENSOR_ATTR_BRIGHT_LIGHT_MODE) {
		data->bl_mode_enabled = val->val1 ? 1 : 0;
		LOG_DBG("requested bright light mode to be %s",
		        data->bl_mode_enabled ? "enabled" : "disabled");
		return 0;
	}
	return -ENOTSUP;
}

static int si1133_attr_get(const struct device *dev, enum sensor_channel chan,
                           enum sensor_attribute attr,
                           struct sensor_value *val)
{
	struct si1133_data *data = dev->data;
	ARG_UNUSED(chan);
	
	if ((int)attr == SENSOR_ATTR_BRIGHT_LIGHT_MODE) {
		val->val1 = data->bl_mode_enabled;
		val->val2 = 0;
		return 0;
	}
	return -ENOTSUP;
}

static int si1133_setup_channel(const struct device *dev,
                                const struct si1133_chan_config *chan_config)
{
	uint8_t chan_list;
	int ret;
	
	/* Read the current CHAN_LIST configuration. */
	ret = si1133_param_query(dev, SI1133_PRM_TBL_CHAN_LIST, &chan_list);
	if (ret < 0) {
		return ret;
	}
	
	/* Set up CHAN_LIST with new configuration. */
	chan_list |= chan_config->enable_chan;
	ret = si1133_param_set(dev, SI1133_PRM_TBL_CHAN_LIST, chan_list);
	if (ret < 0) {
		return ret;
	}
	
	/* Set up ADCCONFIG for this channel. */
	ret = si1133_param_set(dev, chan_config->adcconfig_addr,
	                       chan_config->adcconfig_val);
	if (ret < 0) {
		return ret;
	}
	
	/* Set up ADCSENS for this channel. */
	ret = si1133_param_set(dev, chan_config->adcsens_addr,
	                       chan_config->adcsens_val);
	if (ret < 0) {
		return ret;
	}
	
	/* Set up ADCPOST for this channel. */
	ret = si1133_param_set(dev, chan_config->adcpost_addr,
	                       chan_config->adcpost_val);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

static int si1133_chip_reset(const struct device *dev)
{
	int ret, retry;
	uint8_t rsp0;
	
	if ((ret = si1133_cmd_write(dev, SI1133_CMD_REG_RST_SW)) < 0) {
		LOG_DBG("sw rst cmd write failed");
		return ret;
	}
	/* 
	 * The RESPONSE0 register will show RUNNING immediately after 
	 * reset and then SLEEP after initialization is complete.
	 */
	for (retry = 0; retry < SI1133_VAL_RETRY; retry++)
	{
		k_sleep(K_MSEC(SI1133_VAL_DELAY_MS));
		
		if ((ret = si1133_rsp0_read(dev, &rsp0)) < 0) {
			LOG_DBG("rsp0 read failed");
			return ret;
		}
		if (rsp0 & SI1133_RESPONSE0_BIT_SLEEP) {
			return 0;
		}
	}
	return -ETIMEDOUT;
}

static int si1133_chip_init(const struct device *dev)
{
	struct si1133_chan_config chan_config;
	struct si1133_data *data = dev->data;
	uint8_t part_id;
	int ret;
	
	/* Ensure i2c bus is ready. */
	if ((ret = si1133_bus_check(dev)) < 0) {
		LOG_DBG("bus check failed: %d", ret);
		return ret;
	}
	
	/* Wait for the sensor to be ready for i2c transactions. */
	k_sleep(K_MSEC(SI1133_VAL_STARTUP_TIME_MS));
	
	/* Read and verify PART_ID. */
	ret = si1133_reg_read(dev, SI1133_I2C_REG_PART_ID, &part_id, 1);
	if (ret < 0) {
		LOG_DBG("ID read failed: %d", ret);
		return ret;
	}
	if (part_id == SI1133_VAL_PART_ID) {
		LOG_DBG("ID OK");
	} else {
		LOG_DBG("bad part id 0x%x", part_id);
		return -ENOTSUP;
	}
	
	/* Reset the chip. */
	if ((ret = si1133_chip_reset(dev)) < 0) {
		LOG_DBG("soft reset failed: %d", ret);
		return ret;
	}
	
	/* Set up channel 0 for measuring visible light. */
	chan_config.enable_chan    = SI1133_CFG_ENABLE_CHANNEL_0;
	chan_config.adcconfig_addr = SI1133_PRM_TBL_ADCCONFIG0;
	chan_config.adcconfig_val  = SI1133_CFG_ADCCONFIG0_ADCMUX_WHITE | 
	                             SI1133_CFG_ADCCONFIG0_DECIM_RATE;
	chan_config.adcsens_addr   = SI1133_PRM_TBL_ADCSENS0;
	chan_config.adcsens_val    = SI1133_CFG_ADCSENS0_HW_GAIN;
	chan_config.adcpost_addr   = SI1133_PRM_TBL_ADCPOST0;
	chan_config.adcpost_val    = 0;
	
	if ((ret = si1133_setup_channel(dev, &chan_config)) < 0) {
		LOG_DBG("channel 0 setup failed: %d", ret);
		return ret;
	}
	
	/* Set up channel 1 for measuring infrared light. */
	chan_config.enable_chan    = SI1133_CFG_ENABLE_CHANNEL_1;
	chan_config.adcconfig_addr = SI1133_PRM_TBL_ADCCONFIG1;
	chan_config.adcconfig_val  = SI1133_CFG_ADCCONFIG1_ADCMUX_SMALL_IR | 
	                             SI1133_CFG_ADCCONFIG1_DECIM_RATE;
	chan_config.adcsens_addr   = SI1133_PRM_TBL_ADCSENS1;
	chan_config.adcsens_val    = SI1133_CFG_ADCSENS1_HW_GAIN;
	chan_config.adcpost_addr   = SI1133_PRM_TBL_ADCPOST1;
	chan_config.adcpost_val    = 0;
	
	if ((ret = si1133_setup_channel(dev, &chan_config)) < 0) {
		LOG_DBG("channel 1 setup failed: %d", ret);
		return ret;
	}
	
	/* Set up channel 2 for measuring ultraviolet light. */
	chan_config.enable_chan    = SI1133_CFG_ENABLE_CHANNEL_2;
	chan_config.adcconfig_addr = SI1133_PRM_TBL_ADCCONFIG2;
	chan_config.adcconfig_val  = SI1133_CFG_ADCCONFIG2_ADCMUX_UV | 
	                             SI1133_CFG_ADCCONFIG2_DECIM_RATE;
	chan_config.adcsens_addr   = SI1133_PRM_TBL_ADCSENS2;
	chan_config.adcsens_val    = SI1133_CFG_ADCSENS2_HW_GAIN;
	chan_config.adcpost_addr   = SI1133_PRM_TBL_ADCPOST2;
	chan_config.adcpost_val    = 0;
	
	if ((ret = si1133_setup_channel(dev, &chan_config)) < 0) {
		LOG_DBG("channel 2 setup failed: %d", ret);
		return ret;
	}
	if ((ret = si1133_set_bl_mode(dev, data->bl_mode_enabled)) < 0) {
		LOG_DBG("set bright light mode failed: %d", ret);
		return ret;
	}
	
	LOG_DBG("bright light mode is %s",
	        data->bl_mode_enabled ? "enabled" : "disabled");
	LOG_DBG("\"%s\" OK", dev->name);
	return 0;
}

static const struct sensor_driver_api si1133_api_funcs = {
	.attr_set = si1133_attr_set,
	.attr_get = si1133_attr_get,
	.sample_fetch = si1133_sample_fetch,
	.channel_get = si1133_channel_get,
};

#ifdef CONFIG_PM_DEVICE
static int si1133_pm_action(const struct device *dev,
                            enum pm_device_action action)
{
	int ret = 0;
	
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Re-initialize the chip. */
		ret = si1133_chip_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Reset the chip and stop all ongoing operations. */
		if ((ret = si1133_chip_reset(dev)) < 0) {
			LOG_DBG("suspend failed: %d", ret);
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#define SI1133_DEFINE(inst) \
	static struct si1133_data si1133_data_##inst = { \
		.bl_mode_enabled = 0, \
	}; \
	static const struct si1133_config si1133_config_##inst = { \
		.i2c = I2C_DT_SPEC_INST_GET(inst), \
	}; \
	\
	PM_DEVICE_DT_INST_DEFINE(inst, si1133_pm_action); \
	\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, \
		si1133_chip_init, \
		PM_DEVICE_DT_INST_GET(inst), \
		&si1133_data_##inst, \
		&si1133_config_##inst, \
		POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, \
		&si1133_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(SI1133_DEFINE)
