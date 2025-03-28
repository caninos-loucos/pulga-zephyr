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
                    "Timestamp: %d; Temperature: %d.%02d°C; Pressure: %d.%02d kPa; "
                    "Humidity: %d.%02d %%RH;",
                    bme280_model->timestamp,
                    bme280_model->temperature.val1,
                    bme280_model->temperature.val2 / 10000,
                    bme280_model->pressure.val1,
                    bme280_model->pressure.val2 / 10000,
                    bme280_model->humidity.val1,
                    bme280_model->humidity.val2 / 10000);
}

// Encodes all values of data model into a minimal string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelBME280 * bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "TS%dT%d.%02dP%d.%02dH%d.%02d",
                    bme280_model->timestamp,
                    bme280_model->temperature.val1,
                    bme280_model->temperature.val2 / 10000,
                    bme280_model->pressure.val1,
                    bme280_model->pressure.val2 / 10000,
                    bme280_model->humidity.val1,
                    bme280_model->humidity.val2 / 10000);
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