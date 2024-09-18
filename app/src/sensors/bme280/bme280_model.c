#include <zephyr/logging/log.h>
#include <sensors/bme280/bme280_service.h>

LOG_MODULE_REGISTER(bme280_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI bme280_model_api;
static SensorModelBME280 *bme280_data;

/**
 * IMPLEMENTATIONS
 */

// Encodes each value of data model into a verbose string
static void encode_verbose(uint32_t *data_model, uint8_t *encoded_data, size_t encoded_size)
{
    bme280_data = (SensorModelBME280 *)data_model;

    // Format the string safely
    snprintf(encoded_data, encoded_size,
             "Temperature: %d.%02d oC; Pressure: %d.%02d kPa; "
             "Humidity: %d.%02d %%RH;\n",
             bme280_data->temperature.val1,
             bme280_data->temperature.val2 / 10000,
             bme280_data->pressure.val1,
             bme280_data->pressure.val2 / 10000,
             bme280_data->humidity.val1,
             bme280_data->humidity.val2 / 10000);
}

// Registers BME280 model callbacks
DataAPI *register_bme280_model_callbacks()
{
    bme280_model_api.data_model_words = BME280_MODEL_WORDS;
    bme280_model_api.encode_verbose = encode_verbose;
    // bme280_model_api.encode_minimalist = encode_minimalist;
    // bme280_model_api.split_values = split_values;
    return &bme280_model_api;
}