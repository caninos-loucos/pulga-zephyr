#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <sensors/bme280/bme280_service.h>
#include <data_processing/bme280/bme280_model.h>

LOG_MODULE_REGISTER(bme280_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *bme280;
static SensorAPI bme280_api = {0};
static SensorAPI *bme280_api_ptr = &bme280_api; // Pointer to static instance

/**
 * IMPLEMENTATIONS
 */

// Initializes sensor
static void init_sensor()
{
    LOG_DBG("Initializing BME280");
    bme280 = DEVICE_DT_GET_ANY(bosch_bme280);

    if (!bme280)
    {
        LOG_ERR("bme280 not declared at device tree");
    }
    if (!device_is_ready(bme280))
    {
        LOG_ERR("device \"%s\" is not ready", bme280->name);
        bme280_api_ptr = NULL;
    }
}

// Reads sensor values and stores them in buffer
static void read_sensor_values()
{
    SensorModelBME280 bme280_model;
    uint32_t bme280_data[MAX_32_WORDS];
    int error = 0;

    error = sensor_sample_fetch(bme280);
    if (!error)
    {
        sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP,
                           &bme280_model.temperature);
        sensor_channel_get(bme280, SENSOR_CHAN_PRESS,
                           &bme280_model.pressure);
        sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY,
                           &bme280_model.humidity);
    }
    else
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d",
                "BME280", error);
    }

    memcpy(&bme280_data, &bme280_model, sizeof(SensorModelBME280));

    if (insert_in_buffer(BME280_MODEL, error, bme280_data) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
    }
}

// Register BME280 sensor callbacks
SensorAPI *register_bme280_callbacks()
{
    LOG_DBG("Registering BME280 callbacks");
    bme280_api.init_sensor = init_sensor;
    bme280_api.read_sensor_values = read_sensor_values;
    return bme280_api_ptr;
}