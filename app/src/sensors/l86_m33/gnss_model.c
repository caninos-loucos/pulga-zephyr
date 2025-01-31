#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <sensors/l86_m33/l86_m33_service.h>

LOG_MODULE_REGISTER(gnss_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static DataAPI gnss_model_api;
static SensorModelGNSS *gnss_model;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of data model into a verbose string
static int encode_verbose(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    gnss_model = (SensorModelGNSS *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "Latitude: %d.%d o; Longitude: %d.%d o; Bearing angle: %d.%d o; "
                    "Speed: %d.%d m/s; Altitude: %d.%d m;\n\t"
                    "Timestamp: %02dh %02dmin %02d.%ds - %02d/%02d/20%02d",
                    gnss_model->latitude / 1000000,
                    abs(gnss_model->latitude % 1000000),
                    gnss_model->longitude / 1000000,
                    abs(gnss_model->longitude % 1000000),
                    gnss_model->bearing / 100,
                    gnss_model->bearing % 100,
                    gnss_model->speed / 100,
                    gnss_model->speed % 100,
                    gnss_model->altitude / 100,
                    gnss_model->altitude % 100,
                    gnss_model->real_time.hour,
                    gnss_model->real_time.minute,
                    gnss_model->real_time.millisecond / 1000,
                    gnss_model->real_time.millisecond % 1000,
                    gnss_model->real_time.month_day,
                    gnss_model->real_time.month,
                    gnss_model->real_time.century_year);
}

// Encodes all values of data model into a minimalist string
static int encode_minimalist(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    // Converts words into the model
    gnss_model = (SensorModelGNSS *)data_words;

    // Formats the string
    return snprintf(encoded_data, encoded_size,
                    "LT%dLG%dB%dS%dAL%dTS%02d%02d%dD%02d%02d%02d",
                    gnss_model->latitude,
                    gnss_model->longitude,
                    gnss_model->bearing,
                    gnss_model->speed,
                    gnss_model->altitude,
                    gnss_model->real_time.hour,
                    gnss_model->real_time.minute,
                    gnss_model->real_time.millisecond / 1000,
                    gnss_model->real_time.month_day,
                    gnss_model->real_time.month,
                    gnss_model->real_time.century_year);
}

// Registers Si1133 model callbacks
DataAPI *register_gnss_model_callbacks()
{
    gnss_model_api.num_data_words = GNSS_MODEL_WORDS;
    gnss_model_api.encode_verbose = encode_verbose;
    gnss_model_api.encode_minimalist = encode_minimalist;
    // gnss_model_api.split_values = split_values;
    return &gnss_model_api;
}