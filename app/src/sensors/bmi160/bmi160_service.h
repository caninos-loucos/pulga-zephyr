#ifndef BMI160_SERVICE_H
#define BMI160_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>

typedef struct
{
    struct sensor_value acceleration[3];
    struct sensor_value rotation[3];
} SensorModelBMI160;

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, bmi160 has 2 measurements
// with 3 axis each
#define BMI160_MODEL_WORDS ((sizeof(SensorModelBMI160) + 3) / 4)

// Register BMI160 sensor callbacks
SensorAPI *register_bmi160_callbacks();

// Register BMI160 model callbacks
DataAPI *register_bmi160_model_callbacks();

#endif /* BMI160_SERVICE_H */