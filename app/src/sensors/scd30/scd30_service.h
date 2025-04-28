#ifndef SCD30_SERVICE_H
#define SCD30_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>
#include <integration/data_buffer/buffer_service.h>
typedef struct
{
    uint8_t dataType;
	uint16_t co2; // ppm (parts per million)
	uint16_t temperature; // centidegrees (1 / 100th of a degree Celsius)
	uint8_t humidity; // %RH
	uint32_t timestamp;
} SensorModelSCD30;

// Number of 32-bit words in each data item (model)
#define SCD30_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelSCD30))

// Registers SCD30 model callbacks
DataAPI *register_scd30_model_callbacks();

// Registers SCD30 sensor callbacks
SensorAPI *register_scd30_callbacks();

#endif /* SCD30_SERVICE_H */