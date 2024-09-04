#ifndef SENSORS_INTERFACE_H
#define SENSORS_INTERFACE_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

// #if defined(CONFIG_SENSOR)
// Struct that represents all used sensors
typedef struct
{
    #if defined(CONFIG_BME280)
	struct
	{
		struct sensor_value temperature;
		struct sensor_value pressure;
		struct sensor_value humidity;
		int error;
	} bme280;
    #endif /* CONFIG_BME280 */

    #if defined(CONFIG_SI1133)
	struct
	{
		struct sensor_value light;
		struct sensor_value infrared;
		struct sensor_value uv;
		struct sensor_value uv_index;
		int error;
	} si1133;
    #endif /* CONFIG_SI1133 */
} SensorsData;

typedef struct
{
    #if defined(CONFIG_BME280)
	const struct device *bme280;
    #endif /* CONFIG_BME280 */
    #if defined(CONFIG_SI1133)
	const struct device *si1133;
    #endif /* CONFIG_SI1133 */
} SensorsType;

// Struct containing sensors data and synchronization variables
typedef struct
{
	SensorsData sensors_data;
	struct k_mutex lock;
	struct k_sem data_ready;
} SensorsReturn;

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define SENSORS_THREAD_STACK_SIZE 4096
#define SENSORS_THREAD_PRIORITY 5 /* preemptible */

extern SensorsReturn sensors_return;

// Initializes sensors and start reading
int read_sensors();

// #endif /* CONFIG_SENSOR */
#endif /* SENSORS_INTERFACE_H */