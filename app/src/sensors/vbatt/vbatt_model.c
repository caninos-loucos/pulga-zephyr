#include <zephyr/logging/log.h>
#include <sensors/vbatt/vbatt_service.h>

LOG_MODULE_REGISTER(vbatt_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI vbatt_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelVbatt *vbatt_model = (SensorModelVbatt *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Timestamp: %d; Voltage: %d.%03d V;",
                    vbatt_model->timestamp,
                    vbatt_model->voltage.val1,
                    vbatt_model->voltage.val2 / 1000);
}

// Encodes all values of data model into a minimal string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelVbatt *vbatt_model = (SensorModelVbatt *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "TS%dmV%d",
                    vbatt_model->timestamp,
                    vbatt_model->voltage.val1 * 1000 +
                        vbatt_model->voltage.val2 / 1000);
}

static int encode_raw_bytes(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into bytes
    bytecpy(encoded_data, data_words, encoded_size);

    return sizeof(SensorModelVbatt);
}

// Registers vbatt model callbacks
DataAPI *register_vbatt_model_callbacks()
{
    vbatt_model_api.num_data_words = VBATT_MODEL_WORDS;
    vbatt_model_api.encode_verbose = encode_verbose;
    vbatt_model_api.encode_minimalist = encode_minimalist;
    vbatt_model_api.encode_raw_bytes = encode_raw_bytes;
    // vbatt_model_api.split_values = split_values;
    return &vbatt_model_api;
}