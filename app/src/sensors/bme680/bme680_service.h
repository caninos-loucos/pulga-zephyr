#ifndef BME680_SERVICE_H
#define BME680_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
#include <integration/data_buffer/buffer_service.h>

typedef struct
{
    struct sensor_value temperature;
    struct sensor_value pressure;
    struct sensor_value humidity;
    struct sensor_value gas_resistance;
    uint32_t timestamp;
} SensorModelBME680;

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, bme680 has 4 measurements
#define BME680_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelBME680))

// Register BME680 sensor callbacks
SensorAPI *register_bme680_callbacks();

// Register BME680 model callbacks
DataAPI *register_bme680_model_callbacks();

#endif /* BME680_SERVICE_H */