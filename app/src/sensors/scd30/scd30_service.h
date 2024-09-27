#ifndef SCD30_SERVICE_H
#define SCD30_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, SCD30 has 3 measurements
#define SCD30_MODEL_WORDS 2 * 3
// This define is temporary. Must be adjusted to final code!
#define SAMPLE_PERIOD 3

typedef struct
{
	struct sensor_value co2;
	struct sensor_value temperature;
	struct sensor_value humidity;
} SensorModelSCD30;

// Registers SCD30 model callbacks
DataAPI *register_scd30_model_callbacks();

// Registers SCD30 sensor callbacks
SensorAPI *register_scd30_callbacks();

#endif /* SCD30_SERVICE_H */