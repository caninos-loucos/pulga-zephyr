#ifndef SI1133_SERVICE_H
#define SI1133_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>

#define SI1133_MODEL_WORDS (sizeof(SensorModelSi1133) + 3) / 4

// The Light and Infrared measurements can be stored as either 32- our 16-bit ints, depending on the Bright Light Mode attribute on this sensor.
// For the sake of compatibility, we will leave them as uint32.
// Datasheet link as follows, ref pages 18-19: https://www.alldatasheet.com/datasheet-pdf/view/791376/SILABS/SI1133.html on page
// Further references to the sensor's accuracy are not to be found
typedef struct
{
    uint32_t light;
    uint32_t infrared;
    uint16_t uv;
    uint16_t uv_index;
} SensorModelSi1133;

// Registers Si1133 model callbacks
DataAPI *register_si1133_model_callbacks();

// Registers Si1133 sensor callbacks
SensorAPI *register_si1133_callbacks();

#endif /* SI1133_SERVICE_H */