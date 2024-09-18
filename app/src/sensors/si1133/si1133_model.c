#include <zephyr/logging/log.h>
#include <sensors/si1133/si1133_service.h>

LOG_MODULE_REGISTER(si1133_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI si1133_model_api;
static SensorModelSi1133* si1133_data;

/**
 * IMPLEMENTATIONS
 */

// Encodes each value of data model into a verbose string
static void encode_verbose(uint32_t* data_model, uint8_t* encoded_data, size_t encoded_size){
    si1133_data = (SensorModelSi1133 *)data_model;

    // Format the string safely
    snprintf(encoded_data, encoded_size,
             "Light: %d lux; Infrared: %d lux; UV: %d; UVIndex: %d.%02d;\n",
             si1133_data->light.val1,
             si1133_data->infrared.val1,
             si1133_data->uv.val1,
             si1133_data->uv_index.val1,
             si1133_data->uv_index.val2 / 10000);
}

// Registers Si1133 model callbacks
DataAPI* register_si1133_model_callbacks(){
    si1133_model_api.data_model_words = SI1133_MODEL_WORDS;
    si1133_model_api.encode_verbose = encode_verbose;
    // si1133_model_api.encode_minimalist = encode_minimalist;
    // si1133_model_api.split_values = split_values;
    return &si1133_model_api;
}