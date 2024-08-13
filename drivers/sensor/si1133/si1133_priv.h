/*
 * Copyright (c) 2024 Edgar Bernardi Righi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SI1133_PRIV_H_
#define _SI1133_PRIV_H_

#define SI1133_VAL_STARTUP_TIME_MS				(30)
#define SI1133_VAL_RETRY						(20)
#define SI1133_VAL_DELAY_MS						(5)
#define SI1133_VAL_PART_ID						(0x33)

// I2C register addresses
#define SI1133_I2C_REG_PART_ID					(0x00)
#define SI1133_I2C_REG_COMMAND					(0x0B)
#define SI1133_I2C_REG_RESPONSE0				(0x11)
#define SI1133_I2C_REG_RESPONSE1				(0x10)
#define SI1133_I2C_REG_HOSTIN0					(0x0A)
#define SI1133_I2C_REG_IRQ_STATUS				(0x12)
#define SI1133_I2C_REG_HOSTOUT_BASE				(0x13)

// Register codes associated to commands
#define SI1133_CMD_REG_PRM_SET_PRFX				(0b10000000)
#define SI1133_CMD_REG_PRM_QRY_PRFX				(0b01000000)
// Reset of command counter to 0
#define SI1133_CMD_REG_RST_CMD_CTR				(0x00)
#define SI1133_CMD_REG_RST_SW					(0x01)
#define SI1133_CMD_REG_FORCE					(0x11)

#define SI1133_PRM_TBL_CHAN_LIST				(0x01)
#define SI1133_PRM_TBL_ADCCONFIG0				(0x02)
#define SI1133_PRM_TBL_ADCCONFIG1				(0x06)
#define SI1133_PRM_TBL_ADCCONFIG2				(0x0A)
#define SI1133_PRM_TBL_ADCSENS0					(0x03)
#define SI1133_PRM_TBL_ADCSENS1					(0x07)
#define SI1133_PRM_TBL_ADCSENS2					(0x0B)
#define SI1133_PRM_TBL_ADCPOST0					(0x04)
#define SI1133_PRM_TBL_ADCPOST1					(0x08)
#define SI1133_PRM_TBL_ADCPOST2					(0x0C)

#define SI1133_IRQ_CHANNEL_0					(BIT(0))
#define SI1133_IRQ_CHANNEL_1					(BIT(1))
#define SI1133_IRQ_CHANNEL_2					(BIT(2))

#define SI1133_RESPONSE0_BIT_RUNNING			(BIT(7))
#define SI1133_RESPONSE0_BIT_SUSPEND			(BIT(6))
#define SI1133_RESPONSE0_BIT_SLEEP				(BIT(5))
#define SI1133_RESPONSE0_BIT_CMD_ERR			(BIT(4))
#define SI1133_RESPONSE0_MSK_CMD_CTR			(0xF)

#define SI1133_CFG_ADCCONFIG0_ADCMUX_WHITE		(0b01011)
#define SI1133_CFG_ADCCONFIG1_ADCMUX_SMALL_IR	(0b00000)
#define SI1133_CFG_ADCCONFIG2_ADCMUX_UV			(0b11000)
#define SI1133_CFG_ENABLE_CHANNEL_0				(BIT(0))
#define SI1133_CFG_ENABLE_CHANNEL_1				(BIT(1))
#define SI1133_CFG_ENABLE_CHANNEL_2				(BIT(2))
#define SI1133_CFG_ADCCONFIG0_DECIM_RATE		(0)
#define SI1133_CFG_ADCCONFIG1_DECIM_RATE		(0)
#define SI1133_CFG_ADCCONFIG2_DECIM_RATE		(0x3 << 5)
#define SI1133_CFG_ADCSENS0_HW_GAIN				(0)
#define SI1133_CFG_ADCSENS1_HW_GAIN				(0)
#define SI1133_CFG_ADCSENS2_HW_GAIN				(9)

#define SI1133_CFG_ADCPOSTX_24BIT_OUT			(BIT(6))

#define SI1133_CFG_TOTAL_OUTPUT_BYTES_LL		(6)
#define SI1133_CFG_TOTAL_OUTPUT_BYTES_BL		(8)
#define SI1133_CFG_TOTAL_OUTPUT_BYTES_MAX		(8)

struct si1133_chan_config {
	uint8_t enable_chan;
	uint8_t adcconfig_addr;
	uint8_t adcconfig_val;
	uint8_t adcsens_addr;
	uint8_t adcsens_val;
	uint8_t adcpost_addr;
	uint8_t adcpost_val;
};

struct si1133_data {
	int32_t chan_light;
	int32_t chan_ir;
	int32_t chan_uv;
	int64_t chan_uvi;
	uint8_t bl_mode_enabled;
	uint8_t cmd_counter;
};

struct si1133_config {
	struct i2c_dt_spec i2c;
};

static inline int si1133_bus_check(const struct device *dev) {
	const struct si1133_config *cfg = dev->config;
	return device_is_ready(cfg->i2c.bus) ? 0 : -ENODEV;
}

static inline int si1133_reg_read(const struct device *dev,
                                  uint8_t reg, uint8_t *buf, int size) {
	const struct si1133_config *cfg = dev->config;
	return i2c_burst_read_dt(&cfg->i2c, reg, buf, size);
}

// Write to I2C register 
static inline int si1133_reg_write(const struct device *dev,
                                   uint8_t reg, uint8_t *buf, int size) {
	const struct si1133_config *cfg = dev->config;
	return i2c_burst_write_dt(&cfg->i2c, reg, buf, size);
}

// Write to I2C command register
static inline int si1133_cmd_write(const struct device *dev, uint8_t val) {
	return si1133_reg_write(dev, SI1133_I2C_REG_COMMAND, &val, 1);
}

static inline int si1133_hostin0_write(const struct device *dev,
                                       uint8_t addr, uint8_t val) {
	uint8_t buf[2] = { val, addr };
	return si1133_reg_write(dev, SI1133_I2C_REG_HOSTIN0, buf, 2);
}

static inline int si1133_irq_read(const struct device *dev, uint8_t *val) {
	return si1133_reg_read(dev, SI1133_I2C_REG_IRQ_STATUS, val, 1);
}

// Read RESPONSE0 register
static inline int si1133_rsp0_read(const struct device *dev, uint8_t *val) {
	return si1133_reg_read(dev, SI1133_I2C_REG_RESPONSE0, val, 1);
}

// Read RESPONSE1 register
static inline int si1133_rsp1_read(const struct device *dev, uint8_t *val) {
	return si1133_reg_read(dev, SI1133_I2C_REG_RESPONSE1, val, 1);
}

#endif /* _SI1133_PRIV_H_ */
