#ifndef BME280_SERVICE_H
#define BME280_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
#include <data_processing/data_abstraction.h>

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, bme280 has 3 measurements
#define BME280_MODEL_WORDS 2 * 3

typedef struct
{
    struct sensor_value temperature;
    struct sensor_value pressure;
    struct sensor_value humidity;
} SensorModelBME280;

// Register BME280 sensor callbacks
SensorAPI* register_bme280_callbacks();

// Register BME280 model callbacks
DataAPI* register_bme280_model_callbacks();

#endif /* BME280_SERVICE_H */