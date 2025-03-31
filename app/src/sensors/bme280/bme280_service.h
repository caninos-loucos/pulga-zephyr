#ifndef BME280_SERVICE_H
#define BME280_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
#include <integration/data_buffer/buffer_service.h>

typedef struct
{
    uint8_t dataType;
    int16_t temperature; // centidegrees (1/100th of degree Celsius)
    uint16_t pressure; // hectopascal (hPa)
    uint8_t humidity; // %RH
    uint32_t timestamp;
} SensorModelBME280;

// Number of 32-bit words in each data item (model)
#define BME280_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelBME280))

// Register BME280 sensor callbacks
SensorAPI *register_bme280_callbacks();

// Register BME280 model callbacks
DataAPI *register_bme280_model_callbacks();

#endif /* BME280_SERVICE_H */