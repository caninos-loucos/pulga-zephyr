#include <zephyr/logging/log.h>
#include <zcbor_encode.h>
#include <sensors/bme280/zcbor/bme280_encode.h>
#include <sensors/bme280/bme280_service.h>

LOG_MODULE_REGISTER(bme280_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI bme280_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    SensorModelBME280 *bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Timestamp: %d; Temperature: %d.%02d°C; Pressure: %d.%02d kPa; "
                    "Humidity: %d.%02d %%RH;",
                    bme280_model->timestamp,
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
    SensorModelBME280 *bme280_model = (SensorModelBME280 *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "TS%dT%d.%02dP%d.%02dH%d.%02d",
                    bme280_model->timestamp,
                    bme280_model->temperature.val1,
                    bme280_model->temperature.val2 / 10000,
                    bme280_model->pressure.val1,
                    bme280_model->pressure.val2 / 10000,
                    bme280_model->humidity.val1,
                    bme280_model->humidity.val2 / 10000);
}

static int encode_raw_bytes(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into bytes
    bytecpy(encoded_data, data_words, encoded_size);

    return sizeof(SensorModelBME280);
}

static int encode_cbor(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    SensorModelBME280 *bme280_model = (SensorModelBME280 *)data_words;
    uint32_t zcbor_input = bme280_model->temperature.val1 * 100 + bme280_model->temperature.val1 / 10000;
    uint8_t zcbor_output[MAX_32_WORDS];
    size_t zcbor_output_size;

    int err = cbor_encode_BME280(zcbor_output, sizeof(zcbor_output), &zcbor_input, &zcbor_output_size);
    if (err != ZCBOR_SUCCESS)
    {
        LOG_ERR("Could not encode bme280 data into zcbor, error %d", err);
        return -1;
    }

    zcbor_output[zcbor_output_size] = '\0';
    return snprintf(encoded_data, zcbor_output_size + 9, "{ t %d %s }", zcbor_output_size, zcbor_output);
}

// Registers BME280 model callbacks
DataAPI *register_bme280_model_callbacks()
{
    bme280_model_api.num_data_words = BME280_MODEL_WORDS;
    bme280_model_api.encode_verbose = encode_verbose;
    bme280_model_api.encode_minimalist = encode_minimalist;
    bme280_model_api.encode_cbor = encode_cbor;
    bme280_model_api.encode_raw_bytes = encode_raw_bytes;
    // bme280_model_api.split_values = split_values;
    return &bme280_model_api;
}