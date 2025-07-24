/*
 * Copyright (c) 2024 Kauê Rodrigues Barbosa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SCD30_H_
#define _SCD30_H_

#include <zephyr/drivers/sensor.h>

/* Callback strutucture to be shared in application and driver */
typedef void (*scd30_callback_t)(void);

void scd30_register_callback(const struct device *dev, scd30_callback_t cb);

/* Additional custom attributes */
enum scd30_attribute
{
	/**
	 * Sensor sampling period, i.e. how many seconds there are between
	 * two subsequent measurements.
	 */
	SCD30_SENSOR_ATTR_SAMPLING_PERIOD = SENSOR_ATTR_PRIV_START,
	SCD30_SENSOR_ATTR_TEMPERATURE_OFFSET,
	SCD30_SENSOR_ATTR_ALTITUDE,
	SCD30_SENSOR_ATTR_PRESSURE,
	SCD30_SENSOR_ATTR_AUTO_SELF_CALIBRATION,
	SCD30_SENSOR_ATTR_FORCED_RECALIBRATION,
};

#endif // SCD30_H
