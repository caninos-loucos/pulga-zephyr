#include <zephyr/logging/log.h>
#include <sensors/si1133/si1133_service.h>

LOG_MODULE_REGISTER(si1133_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI si1133_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelSi1133 *si1133_model = (SensorModelSi1133 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Timestamp: %d; Light: %d lux; Infrared: %d lux; UV: %d; "
                    "UVIndex: %d.%02d;",
                    si1133_model->timestamp,
                    si1133_model->light.val1,
                    si1133_model->infrared.val1,
                    si1133_model->uv.val1,
                    si1133_model->uv_index.val1,
                    si1133_model->uv_index.val2 / 10000);
}

// Encodes all values of data model into a minimalist string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelSi1133 *si1133_model = (SensorModelSi1133 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "TS%dL%dIR%dUV%dI%d.%02d",
                    si1133_model->timestamp,
                    si1133_model->light.val1,
                    si1133_model->infrared.val1,
                    si1133_model->uv.val1,
                    si1133_model->uv_index.val1,
                    si1133_model->uv_index.val2 / 10000);
}

static int encode_raw_bytes(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into bytes
    bytecpy(encoded_data, data_words, encoded_size);

    return sizeof(SensorModelSi1133);
}

// Registers Si1133 model callbacks
DataAPI *register_si1133_model_callbacks()
{
    si1133_model_api.num_data_words = SI1133_MODEL_WORDS;
    si1133_model_api.encode_verbose = encode_verbose;
    si1133_model_api.encode_minimalist = encode_minimalist;
    si1133_model_api.encode_raw_bytes = encode_raw_bytes;
    // si1133_model_api.split_values = split_values;
    return &si1133_model_api;
}