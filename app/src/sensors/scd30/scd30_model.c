#include <zephyr/logging/log.h>
#include <sensors/scd30/scd30_service.h>

LOG_MODULE_REGISTER(scd30_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI scd30_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelSCD30 *scd30_model = (SensorModelSCD30 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Timestamp: %d; CO2: %d ppm; Temperature: %d.%02d oC; "
                    "Humidity: %d.%02d %% RH;",
                    scd30_model->timestamp,
                    scd30_model->co2.val1,
                    scd30_model->temperature.val1,
                    scd30_model->temperature.val2 / 10000,
                    scd30_model->humidity.val1,
                    scd30_model->humidity.val2 / 10000);
}

// Encodes all values of data model into a mininal string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelSCD30 *scd30_model = (SensorModelSCD30 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "TS%dCO2%dT%d.%02dH%d.%02d",
                    scd30_model->timestamp,
                    scd30_model->co2.val1,
                    scd30_model->temperature.val1,
                    scd30_model->temperature.val2 / 10000,
                    scd30_model->humidity.val1,
                    scd30_model->humidity.val2 / 10000);
}

static int encode_raw_bytes(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into bytes
    bytecpy(encoded_data, data_words, encoded_size);

    return sizeof(SensorModelSCD30);
}

// Registers SCD30 model callbacks
DataAPI *register_scd30_model_callbacks()
{
    scd30_model_api.num_data_words = SCD30_MODEL_WORDS;
    scd30_model_api.encode_verbose = encode_verbose;
    scd30_model_api.encode_minimalist = encode_minimalist;
    scd30_model_api.encode_raw_bytes = encode_raw_bytes;
    //  scd30_model_api.split_values = split_values;
    return &scd30_model_api;
}