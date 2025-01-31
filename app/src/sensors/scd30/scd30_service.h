#ifndef SCD30_SERVICE_H
#define SCD30_SERVICE_H

#include <zephyr/drivers/sensor.h>
#include <sensors/sensors_interface.h>

// Number of 32-bit words in each data item (model)
#define SCD30_MODEL_WORDS (sizeof(SensorModelSCD30) + 3) / 4

// The real values are quantized according to the sensor accuracy and range, which are available
// on the product catalog and datasheet at https://sensirion.com/products/catalog/SCD30.
// - Humidity: 0-100%RH ±3%RH
// - Temperature: 0-50°C ±0.4°C
// - CO2: 0-10000 ppm ±(30 + 3% * measured value ) ppm
//  This all fits perfectly in a word
typedef struct
{
	uint16_t co2;
	uint16_t temperature;
	uint8_t humidity;
} SensorModelSCD30;

// Registers SCD30 model callbacks
DataAPI *register_scd30_model_callbacks();

// Registers SCD30 sensor callbacks
SensorAPI *register_scd30_callbacks();

#endif /* SCD30_SERVICE_H */