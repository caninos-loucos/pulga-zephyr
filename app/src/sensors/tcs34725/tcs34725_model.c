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
    return snprintf(encoded_data, encoded_size,
                    "Clear: %04X; Red: %04X; Green: %04X; "
                    "Blue: %04X; Luminosity: %d.%02d lux; Color Temperature: %d K",
                    tcs34725_model->clear.val1,
                    tcs34725_model->red.val1,
                    tcs34725_model->green.val1,
                    tcs34725_model->blue.val1,
                    tcs34725_model->luminosity.val1,
                    tcs34725_model->luminosity.val2 / 10000,
                    tcs34725_model->color_temperature.val1);
    return 0;
}

// Encodes all values of data model into a mininal string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelTCS34725 *tcs34725_model = (SensorModelTCS34725 *)data_words;

    // In production. Will change this later!
    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "C%04XR%04XG%04XB%04XL%d.%02dCT%d",
                    tcs34725_model->clear.val1,
                    tcs34725_model->red.val1,
                    tcs34725_model->green.val1,
                    tcs34725_model->blue.val1,
                    tcs34725_model->luminosity.val1,
                    tcs34725_model->luminosity.val2 / 10000,
                    tcs34725_model->color_temperature.val1);
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