#ifndef SENSORS_INTERFACE_H
#define SENSORS_INTERFACE_H

#include <data_processing/data_abstraction.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define SENSORS_THREAD_STACK_SIZE 1024
#define SENSORS_THREAD_PRIORITY 5 /* preemptible */

// Encoding used to map sensors APIs
enum SensorType {
    BME280,
    BMI160,
    SI1133,
    SCD30,
	MAX_SENSORS // Total number of sensors
};

// Functions exposed for each sensor
typedef struct{
	// Initializes sensor
	void (*init_sensor)();
	// Reads sensor values and stores them in buffer
	void (*read_sensor_values)();
} SensorAPI;

extern SensorAPI* sensors_apis[MAX_SENSORS];

// TODO: probably will require sync
// Dynamically sets current sampling interval
void set_sampling_interval(int new_interval);
// Gets current sampling interval
int get_sampling_interval();
// Registers callbacks for the used sensors
int register_sensors_callbacks();
// Initializes sensors and start reading
int read_sensors();

#endif /* SENSORS_INTERFACE_H */