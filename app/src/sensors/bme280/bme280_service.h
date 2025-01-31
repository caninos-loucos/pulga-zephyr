#ifndef BME280_SERVICE_H
#define BME280_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>

// Number of 32-bit words in each data item (model)
#define BME280_MODEL_WORDS (sizeof(SensorModelBME280) + 3) / 4

typedef struct
{
    int16_t temperature;
    uint16_t pressure;
    uint8_t humidity;
} SensorModelBME280;

// Register BME280 sensor callbacks
SensorAPI *register_bme280_callbacks();

// Register BME280 model callbacks
DataAPI *register_bme280_model_callbacks();

#endif /* BME280_SERVICE_H */