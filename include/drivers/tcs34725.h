/*
 * Copyright (c) 2025 Kauê Rodrigues Barbosa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TCS34725_H_
#define TCS34725_H_

#include <zephyr/drivers/sensor.h>

/* TCS34725 specific channels */
enum sensor_channel_tcs34725 {
    // Not sure yet if will be needed
	SENSOR_CHAN_TEMPORARY = SENSOR_CHAN_PRIV_START,
};

/* TCS34725 specific attribute */
enum sensor_attribute_tcs34725 {
	SENSOR_ATTR_RGBC_INTERRUPT = SENSOR_ATTR_PRIV_START,
    // In production 
    // SENSOR_ATTR_WAIT_TIME,
    // SENSOR_ATTR_RGBC_STATUS,
    // SENSOR_ATTR_POWER_ON,
    // SENSOR_ATTR_GAIN,
    // SENSOR_ATTR_CHIP_REGISTER,
    // SENSOR_ATTR_RGBC_INTEGRATION_TIME,
    // SENSOR_ATTR_WAIT, // Needs revision
    // SENSOR_ATTR_RGBC_CLEAR_LOW_INTERRUPT_THRESHOLD, // Needs revision
    // SENSOR_ATTR_RGBC_CLEAR_HIGH_INTERRUPT_THRESHOLD, // Needs revision
    // SENSOR_ATTR_INTERRUPT_MODE, // Maybe zephyr's CONFIGURATION attr is enough for this one
};

#endif

