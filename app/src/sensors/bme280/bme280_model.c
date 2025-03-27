#include <zephyr/logging/log.h>
#include <sensors/bme280/bme280_service.h>

LOG_MODULE_REGISTER(bme280_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI bme280_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelBME280 *bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Temperature: %d.%02doC; Pressure: %d.%d hPa; "
                    "Humidity: %d %%RH;",
                    bme280_model->temperature / 100,
                    bme280_model->temperature % 100,
                    bme280_model->pressure / 10,
                    bme280_model->pressure % 10,
                    bme280_model->humidity);
}

// Encodes all values of data model into a minimal string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelBME280 * bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "T%dP%dH%d",
                    bme280_model->temperature,
                    bme280_model->pressure,
                    bme280_model->humidity);
}

static int encode_raw_bytes(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into bytes
    bytecpy(encoded_data, data_words, encoded_size);

    return sizeof(SensorModelBME280);
}

// Registers BME280 model callbacks
DataAPI *register_bme280_model_callbacks()
{
    bme280_model_api.num_data_words = BME280_MODEL_WORDS;
    bme280_model_api.encode_verbose = encode_verbose;
    bme280_model_api.encode_minimalist = encode_minimalist;
    bme280_model_api.encode_raw_bytes = encode_raw_bytes;
    // bme280_model_api.split_values = split_values;
    return &bme280_model_api;
}