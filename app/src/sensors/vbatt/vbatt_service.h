#ifndef VBATT_SERVICE_H
#define VBATT_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
#include <integration/data_buffer/buffer_service.h>

typedef struct
{
    struct sensor_value millivolts;
    uint32_t timestamp;
} SensorModelVbatt;

// Number of 32-bit words in each data item (model)
// Each sensor_value has 2 words, vbatt has 3 measurements
#define VBATT_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelVbatt))

// Register vbatt sensor callbacks
SensorAPI *register_vbatt_callbacks();

// Register vbatt model callbacks
DataAPI *register_vbatt_model_callbacks();

#endif /* vbatt_SERVICE_H */