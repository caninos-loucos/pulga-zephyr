#ifndef SI1133_SERVICE_H
#define SI1133_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
typedef struct
{
    uint32_t light;
    uint32_t infrared;
    uint16_t uv;
    uint16_t uv_index;
} SensorModelSi1133;

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, si1133 has 4 measurements
#define SI1133_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelSi1133))

// Registers Si1133 model callbacks
DataAPI *register_si1133_model_callbacks();

// Registers Si1133 sensor callbacks
SensorAPI *register_si1133_callbacks();

#endif /* SI1133_SERVICE_H */