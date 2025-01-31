#include <zephyr/logging/log.h>
#include <sensors/scd30/scd30_service.h>

LOG_MODULE_REGISTER(scd30_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI scd30_model_api;
static SensorModelSCD30 *scd30_model;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    scd30_model = (SensorModelSCD30 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "CO2: %d ppm; Temperature: %d.%02d oC; Humidity: %d %%RH;",
                    scd30_model->co2,
                    scd30_model->temperature / 100,
                    scd30_model->temperature % 100,
                    scd30_model->humidity);
}

// Encodes all values of data model into a mininal string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    scd30_model = (SensorModelSCD30 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "CO2%dT%dH%d",
                    scd30_model->co2,
                    scd30_model->temperature,
                    scd30_model->humidity);
}

// Registers SCD30 model callbacks
DataAPI *register_scd30_model_callbacks()
{
    scd30_model_api.num_data_words = SCD30_MODEL_WORDS;
    scd30_model_api.encode_verbose = encode_verbose;
    scd30_model_api.encode_minimalist = encode_minimalist;
    //  scd30_model_api.split_values = split_values;
    return &scd30_model_api;
}