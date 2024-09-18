#ifndef SI1133_SERVICE_H
#define SI1133_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, si1133 has 4 measurements
#define SI1133_MODEL_WORDS 2 * 4

typedef struct
{
    struct sensor_value light;
    struct sensor_value infrared;
    struct sensor_value uv;
    struct sensor_value uv_index;
} SensorModelSi1133;

// Registers Si1133 model callbacks
DataAPI *register_si1133_model_callbacks();

// Registers Si1133 sensor callbacks
SensorAPI *register_si1133_callbacks();

#endif /* SI1133_SERVICE_H */