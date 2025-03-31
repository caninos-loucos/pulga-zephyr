#include <zephyr/logging/log.h>
#include <sensors/tcs34725/tcs34725_service.h>

LOG_MODULE_REGISTER(tcs34725_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI tcs34725_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelTCS34725 *tcs34725_model = (SensorModelTCS34725 *)data_words;

    //  In production. Will change this later!
    // Formats the string
    // return snprintf(encoded_data, encoded_size,
    //                 "Timestamp: %d; CO2: %d ppm; Temperature: %d.%02d oC; "
    //                 "Humidity: %d.%02d %% RH;",
    //                 tcs34725_model->timestamp,
    //                 tcs34725_model->co2.val1,
    //                 tcs34725_model->temperature.val1,
    //                 tcs34725_model->temperature.val2 / 10000,
    //                 tcs34725_model->humidity.val1,
    //                 tcs34725_model->humidity.val2 / 10000);
    return 0;
}

// Encodes all values of data model into a mininal string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelTCS34725 *tcs34725_model = (SensorModelTCS34725 *)data_words;

    // In production. Will change this later!
    // Formats the string
    // return snprintf(encoded_data, encoded_size,
    //                 "TS%dCO2%dT%d.%02dH%d.%02d",
    //                 tcs34725_model->timestamp,
    //                 tcs34725_model->co2.val1,
    //                 tcs34725_model->temperature.val1,
    //                 tcs34725_model->temperature.val2 / 10000,
    //                 tcs34725_model->humidity.val1,
    //                 tcs34725_model->humidity.val2 / 10000);
    return 0;
}

static int encode_raw_bytes(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into bytes
    bytecpy(encoded_data, data_words, encoded_size);

    return sizeof(SensorModelTCS34725);
}

// Registers TCS34725 model callbacks
DataAPI *register_tcs34725_model_callbacks()
{
    tcs34725_model_api.num_data_words = TCS34725_MODEL_WORDS;
    tcs34725_model_api.encode_verbose = encode_verbose;
    tcs34725_model_api.encode_minimalist = encode_minimalist;
    tcs34725_model_api.encode_raw_bytes = encode_raw_bytes;
    //  tcs34725_model_api.split_values = split_values;
    return &tcs34725_model_api;
}