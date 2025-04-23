#ifndef TCS34725_SERVICE_H
#define TCS34725_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
#include <integration/data_buffer/buffer_service.h>


typedef struct
{
	struct sensor_value clear;
	struct sensor_value red;
	struct sensor_value green;
	struct sensor_value blue;
	struct sensor_value luminosity;
	struct sensor_value color_temperature;
} SensorModelTCS34725;

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, TCS34725 has 3 measurements
#define TCS34725_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelTCS34725))

// Registers TCS34725 model callbacks
DataAPI *register_tcs34725_model_callbacks();

// Registers TCS34725 sensor callbacks
SensorAPI *register_tcs34725_callbacks();

#endif /* TCS34725_SERVICE_H */