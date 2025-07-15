#include <zephyr/logging/log.h>
#include <sensors/bme680/bme680_service.h>

LOG_MODULE_REGISTER(bme680_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI bme680_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelBME680 *bme680_model = (SensorModelBME680 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Timestamp: %d; Temperature: %d.%02d oC; Pressure: %d.%02d kPa; "
                    "Humidity: %d.%02d %%RH; Gas Resistance: %d ohms",
                    bme680_model->timestamp,
                    bme680_model->temperature.val1,
                    bme680_model->temperature.val2 / 10000,
                    bme680_model->pressure.val1,
                    bme680_model->pressure.val2 / 10000,
                    bme680_model->humidity.val1,
                    bme680_model->humidity.val2 / 10000,
                    bme680_model->gas_resistance.val1);
}

// Encodes all values of data model into a minimal string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelBME680 * bme680_model = (SensorModelBME680 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "TS%dT%d.%02dP%d.%02dH%d.%02dGR%d",
                    bme680_model->timestamp,
                    bme680_model->temperature.val1,
                    bme680_model->temperature.val2 / 10000,
                    bme680_model->pressure.val1,
                    bme680_model->pressure.val2 / 10000,
                    bme680_model->humidity.val1,
                    bme680_model->humidity.val2 / 10000,
                    bme680_model->gas_resistance.val1);
}

static int encode_raw_bytes(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into bytes
    bytecpy(encoded_data, data_words, encoded_size);

    return sizeof(SensorModelBME680);
}

// Registers BME680 model callbacks
DataAPI *register_bme680_model_callbacks()
{
    bme680_model_api.num_data_words = BME680_MODEL_WORDS;
    bme680_model_api.encode_verbose = encode_verbose;
    bme680_model_api.encode_minimalist = encode_minimalist;
    bme680_model_api.encode_raw_bytes = encode_raw_bytes;
    // bme680_model_api.split_values = split_values;
    return &bme680_model_api;
}