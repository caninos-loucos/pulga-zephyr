/*
 * Copyright (c) 2024 Edgar Bernardi Righi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SCD30_H_
#define _SCD30_H_

#include <zephyr/drivers/sensor.h>

/* Additional custom attributes */
enum scd30_attribute {
	/**
	 * Sensor sampling period, i.e. how many seconds there are between
	 * two subsequent measurements.
	 */
	SCD30_SENSOR_ATTR_SAMPLING_PERIOD = SENSOR_ATTR_PRIV_START,
};


#endif // SCD30_H
