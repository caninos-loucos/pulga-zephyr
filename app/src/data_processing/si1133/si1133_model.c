#include <zephyr/logging/log.h>
#include <data_processing/si1133/si1133_model.h>

LOG_MODULE_REGISTER(si1133_model, CONFIG_APP_LOG_LEVEL);


/**
 * DEFINITIONS
 */

static DataAPI si1133_model_api;
static SensorModelSi1133* si1133_data;

/**
 * IMPLEMENTATIONS
 */

// Correctly parses data from buffer
void parse_buffer_data(uint32_t* data_model){
    // LOG_DBG("Parsing data");
    uint16_t type;
    uint8_t error = 0, size_words = SI1133_MODEL_WORDS;
    bool discarding = 0;

    // Discarding oldest data
    if(data_model == NULL){
        discarding = 1;
        LOG_DBG("Discarding data item");
    }

    if(!ring_buf_item_get(&data_buffer, &type, &error, data_model, &size_words)){
        if(discarding){
            return;
        }
        if (error) {
            LOG_ERR("fetch sample from \"%s\" failed: %d", "Si1133", error);
        }
    } else {
        LOG_ERR("Failed to get data from ring buffer.");
        data_model = NULL;
    }
}

// Encodes each value of data model into a verbose string
void encode_verbose(uint32_t* data_model, uint8_t* encoded_data, size_t encoded_size){
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
    si1133_model_api.parse_buffer_data = parse_buffer_data;
    si1133_model_api.encode_verbose = encode_verbose;
    // si1133_model_api.encode_minimalist = encode_minimalist;
    // si1133_model_api.split_values = split_values;
    return &si1133_model_api;
}