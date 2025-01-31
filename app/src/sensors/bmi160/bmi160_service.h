#ifndef BMI160_SERVICE_H
#define BMI160_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, bmi160 has 2 measurements
// with 3 axis each
#define BMI160_MODEL_WORDS (sizeof(SensorModelBMI160) + 3) / 4

typedef struct
{
    int16_t acceleration[3];
    int16_t rotation[3];
} SensorModelBMI160;

// Register BMI160 sensor callbacks
SensorAPI *register_bmi160_callbacks();

// Register BMI160 model callbacks
DataAPI *register_bmi160_model_callbacks();

#endif /* BMI160_SERVICE_H */