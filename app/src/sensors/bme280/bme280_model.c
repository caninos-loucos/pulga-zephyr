#include <zephyr/logging/log.h>
#include <sensors/bme280/bme280_service.h>

LOG_MODULE_REGISTER(bme280_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI bme280_model_api;
static SensorModelBME280 *bme280_model;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Temperature: %d.%02d°C; Pressure: %d.%02d kPa; "
                    "Humidity: %d.%02d %%RH;",
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
    bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "T%d.%02dP%d.%02dH%d.%02d",
                    bme280_model->temperature.val1,
                    bme280_model->temperature.val2 / 10000,
                    bme280_model->pressure.val1,
                    bme280_model->pressure.val2 / 10000,
                    bme280_model->humidity.val1,
                    bme280_model->humidity.val2 / 10000);
}

static int encode_compressed(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    bme280_model = (SensorModelBME280 *)data_words;
    LOG_INF("compressing thing");
    return LZ4_compress_default((char *)bme280_model, (char *)encoded_data, BME280_MODEL_WORDS * 4, encoded_size);
}

// Registers BME280 model callbacks
DataAPI *register_bme280_model_callbacks()
{
    bme280_model_api.num_data_words = BME280_MODEL_WORDS;
    bme280_model_api.encode_verbose = encode_verbose;
    bme280_model_api.encode_minimalist = encode_minimalist;
    bme280_model_api.encode_compressed = encode_compressed;
    // bme280_model_api.split_values = split_values;
    return &bme280_model_api;
}