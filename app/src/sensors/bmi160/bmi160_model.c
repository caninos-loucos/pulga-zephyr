#include <zephyr/logging/log.h>
#include <sensors/bmi160/bmi160_service.h>

LOG_MODULE_REGISTER(bmi160_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI bmi160_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t data_size, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelBMI160 *bmi160_model = (SensorModelBMI160 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Timestamp: %d; "
                    "Acceleration [m/s²]: %d.%02d (X) %d.%02d (Y) %d.%02d (Z); "
                    "Rotation [radian/s]: %d.%02d (X) %d.%02d (Y) %d.%02d (Z);",
                    bmi160_model->timestamp,
                    bmi160_model->acceleration[0].val1,
                    bmi160_model->acceleration[0].val2 / 10000,
                    bmi160_model->acceleration[1].val1,
                    bmi160_model->acceleration[1].val2 / 10000,
                    bmi160_model->acceleration[2].val1,
                    bmi160_model->acceleration[2].val2 / 10000,
                    bmi160_model->rotation[0].val1,
                    bmi160_model->rotation[0].val2 / 10000,
                    bmi160_model->rotation[1].val1,
                    bmi160_model->rotation[1].val2 / 10000,
                    bmi160_model->rotation[2].val1,
                    bmi160_model->rotation[2].val2 / 10000);
}

// Encodes all values of data model into a minimalist string
static int encode_minimalist(uint32_t *data_words, uint8_t data_size, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelBMI160 *bmi160_model = (SensorModelBMI160 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "TS%dAC%d.%02d %d.%02d %d.%02dR%d.%02d %d.%02d %d.%02d",
                    bmi160_model->timestamp,
                    bmi160_model->acceleration[0].val1,
                    bmi160_model->acceleration[0].val2 / 10000,
                    bmi160_model->acceleration[1].val1,
                    bmi160_model->acceleration[1].val2 / 10000,
                    bmi160_model->acceleration[2].val1,
                    bmi160_model->acceleration[2].val2 / 10000,
                    bmi160_model->rotation[0].val1,
                    bmi160_model->rotation[0].val2 / 10000,
                    bmi160_model->rotation[1].val1,
                    bmi160_model->rotation[1].val2 / 10000,
                    bmi160_model->rotation[2].val1,
                    bmi160_model->rotation[2].val2 / 10000);
}

static int encode_raw_bytes(uint32_t *data_words, uint8_t data_size, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into bytes
    bytecpy(encoded_data, data_words, encoded_size);

    return sizeof(SensorModelBMI160);
}

// Registers BMI160 model callbacks
DataAPI *register_bmi160_model_callbacks()
{
    bmi160_model_api.num_data_words = BMI160_MODEL_WORDS;
    bmi160_model_api.encode_verbose = encode_verbose;
    bmi160_model_api.encode_minimalist = encode_minimalist;
    bmi160_model_api.encode_raw_bytes = encode_raw_bytes;
    // bmi160_model_api.split_values = split_values;
    return &bmi160_model_api;
}