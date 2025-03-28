#ifndef BMI160_SERVICE_H
#define BMI160_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
#include <integration/data_buffer/buffer_service.h>

typedef struct
{
    struct sensor_value acceleration[3];
    struct sensor_value rotation[3];
    uint32_t timestamp;
} SensorModelBMI160;

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, bmi160 has 2 measurements
// with 3 axis each
#define BMI160_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelBMI160))

// Register BMI160 sensor callbacks
SensorAPI *register_bmi160_callbacks();

// Register BMI160 model callbacks
DataAPI *register_bmi160_model_callbacks();

#endif /* BMI160_SERVICE_H */