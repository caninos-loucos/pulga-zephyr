/*
 * Copyright (c) 2024 Edgar Bernardi Righi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SI1133_H_
#define _SI1133_H_

#include <zephyr/drivers/sensor.h>

/* SI1133 specific channels */
enum sensor_channel_si1133 {
	SENSOR_CHAN_UV = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_UVI,
};

/* SI1133 specific attribute */
enum sensor_attribute_si1133 {
	/* 
	 * Disabled: sensor operating under low-light (e.g. indoor) conditions.
	 * Enabled: sensor operating unde bright-light (e.g. sunlight) conditions.
	 */
	SENSOR_ATTR_BRIGHT_LIGHT_MODE = SENSOR_ATTR_PRIV_START,
};

#endif

