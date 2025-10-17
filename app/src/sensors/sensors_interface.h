#ifndef SENSORS_INTERFACE_H
#define SENSORS_INTERFACE_H

#include <integration/data_abstraction/abstraction_service.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define SENSORS_THREAD_STACK_SIZE 1024
#define SENSORS_THREAD_PRIORITY 5 /* preemptible */

// Encoding used to map sensors APIs
// **Sensors must be on the same order as in DataType enum**
enum SensorType
{
	BME280,
	BMI160,
	SI1133,
	VBATT,
	SCD30,
	L86_M33,	// GNSS Sensor
	MAX_SENSORS // Total number of sensors
};

// Functions exposed for each sensor
typedef struct
{
	// Initializes sensor
	int (*init_sensor)();
	// Reads sensor values and stores them in buffer
	void (*read_sensor_values)();
	// Suspend periodic measurements on the sensor
	void (*suspend_periodic_measurement)();
	// Resume periodic measurements on the sensor, to be used after a stop
	void (*resume_periodic_measurement)();
	// Data processing API
	DataAPI *data_model_api;
} SensorAPI;

// List of registered sensor APIs
extern SensorAPI *sensor_apis[MAX_SENSORS];

// Registers callbacks for the used sensors
int register_sensors_callbacks();
// Initializes sensors and start reading them
int read_sensors();
// #TODO: probably will require sync
// Set the interval in milliseconds between samples
void set_sampling_interval(int new_interval);
// Get the interval in milliseconds between samples
int get_sampling_interval();
// Suspend the periodic read of a sensor
int suspend_sensor_read(char *sensor_name);
// Resume the periodic read of a sensor
int resume_sensor_read(char *sensor_name);

#endif /* SENSORS_INTERFACE_H */