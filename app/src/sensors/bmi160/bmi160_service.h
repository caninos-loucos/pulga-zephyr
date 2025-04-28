#ifndef BMI160_SERVICE_H
#define BMI160_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
#include <integration/data_buffer/buffer_service.h>

typedef struct
{
    uint8_t dataType;
    int16_t acceleration[3]; // cm/s²
    int16_t rotation[3]; // milliradians/s
    uint32_t timestamp; 
} SensorModelBMI160;

// Number of 32-bit words in each data item (model)
#define BMI160_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelBMI160))

// Register BMI160 sensor callbacks
SensorAPI *register_bmi160_callbacks();

// Register BMI160 model callbacks
DataAPI *register_bmi160_model_callbacks();

#endif /* BMI160_SERVICE_H */