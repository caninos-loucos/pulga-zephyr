#include <zephyr/logging/log.h>
#include <data_processing/bme280/bme280_model.h>

LOG_MODULE_REGISTER(bme280_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI bme280_model_api;
static SensorModelBME280 *bme280_data;

/**
 * IMPLEMENTATIONS
 */

// Correctly parses data from buffer
static void parse_buffer_data(uint32_t *data_model)
{
    // LOG_DBG("Parsing data");
    uint16_t type;
    uint8_t error = 0, size_words = BME280_MODEL_WORDS;
    bool discarding = 0;

    // Discarding oldest data
    if (data_model == NULL)
    {
        discarding = 1;
        LOG_DBG("Discarding data item");
    }

    if (!ring_buf_item_get(&data_buffer, &type, &error, data_model, &size_words))
    {
        if (discarding)
        {
            return;
        }
        if (error)
        {
            LOG_ERR("Error reading from \"%s\": %d", "BME280", error);
        }
        LOG_DBG("Got item from buffer starting with '0x%X' and ending with '0x%X'",
                data_model[0], data_model[size_words - 1]);
    }
    else
    {
        LOG_ERR("Failed to get data from ring buffer.");
        data_model = NULL;
    }
}

// Encodes each value of data model into a verbose string
static void encode_verbose(uint32_t *data_model, uint8_t *encoded_data, size_t encoded_size)
{
    bme280_data = (SensorModelBME280 *)data_model;

    // Format the string safely
    snprintf(encoded_data, encoded_size,
             "Temperature: %d.%02d oC; Pressure: %d.%02d kPa; "
             "Humidity: %d.%02d %%RH;\n",
             bme280_data->temperature.val1,
             bme280_data->temperature.val2 / 10000,
             bme280_data->pressure.val1,
             bme280_data->pressure.val2 / 10000,
             bme280_data->humidity.val1,
             bme280_data->humidity.val2 / 10000);
}

// Registers BME280 model callbacks
DataAPI *register_bme280_model_callbacks()
{
    bme280_model_api.parse_buffer_data = parse_buffer_data;
    bme280_model_api.encode_verbose = encode_verbose;
    // bme280_model_api.encode_minimalist = encode_minimalist;
    // bme280_model_api.split_values = split_values;
    return &bme280_model_api;
}