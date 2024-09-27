#include <zephyr/logging/log.h>
#include <sensors/bme280/bme280_service.h>

LOG_MODULE_REGISTER(bme280_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI bme280_model_api;
static SensorModelBME280 *bme280_model;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static void encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    snprintf(encoded_data, encoded_size,
             "Temperature: %d.%02d°C; Pressure: %d.%02d kPa; "
             "Humidity: %d.%02d %%RH;\n",
             bme280_model->temperature.val1,
             bme280_model->temperature.val2 / 10000,
             bme280_model->pressure.val1,
             bme280_model->pressure.val2 / 10000,
             bme280_model->humidity.val1,
             bme280_model->humidity.val2 / 10000);
}

// Encodes all values of data model into a minimal string
static void encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    snprintf(encoded_data, encoded_size,
             "T%d.%02dC P%d.%02d H%d.%02d\n",
             bme280_model->temperature.val1,
             bme280_model->temperature.val2 / 10000,
             bme280_model->pressure.val1,
             bme280_model->pressure.val2 / 10000,
             bme280_model->humidity.val1,
             bme280_model->humidity.val2 / 10000);
}

// Registers BME280 model callbacks
DataAPI *register_bme280_model_callbacks()
{
    bme280_model_api.num_data_words = BME280_MODEL_WORDS;
    bme280_model_api.encode_verbose = encode_verbose;
    bme280_model_api.encode_minimalist = encode_minimalist;
    // bme280_model_api.split_values = split_values;
    return &bme280_model_api;
}