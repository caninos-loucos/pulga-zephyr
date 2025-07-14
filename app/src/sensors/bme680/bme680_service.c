#include <integration/timestamp/timestamp_service.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <sensors/bme680/bme680_service.h>

LOG_MODULE_REGISTER(bme680_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *bme680;
static SensorAPI bme680_api = {0};

/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing BME680");
    bme680 = DEVICE_DT_GET_ANY(bosch_bme680);

    // Removes sensor API from registered APIs if cannot start sensor
    if (!bme680)
    {
        LOG_ERR("bme680 not declared at device tree");
        return -ENODEV;
    }
    else if (!device_is_ready(bme680))
    {
        LOG_ERR("device \"%s\" is not ready", bme680->name);
        return -EAGAIN;
    }
    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading BME680");

    SensorModelBME680 bme680_model;
    uint32_t bme680_data[MAX_32_WORDS];
    int error;

sample_fetch:
    error = sensor_sample_fetch(bme680);
    if (!error)
    {
        sensor_channel_get(bme680, SENSOR_CHAN_AMBIENT_TEMP,
                           &bme680_model.temperature);
        sensor_channel_get(bme680, SENSOR_CHAN_PRESS,
                           &bme680_model.pressure);
        sensor_channel_get(bme680, SENSOR_CHAN_HUMIDITY,
                           &bme680_model.humidity);
        sensor_channel_get(bme680, SENSOR_CHAN_GAS_RES,
                           &bme680_model.gas_resistance);
#ifndef CONFIG_EVENT_TIMESTAMP_NONE
        bme680_model.timestamp = get_current_timestamp();
#endif /* CONFIG_EVENT_TIMESTAMP_NONE */

        memcpy(&bme680_data, &bme680_model, sizeof(SensorModelBME680));

        if (insert_in_buffer(&app_buffer, bme680_data, BME680_MODEL, error, BME680_MODEL_WORDS) != 0)
        {
            LOG_ERR("Failed to insert data in ring buffer.");
        }
    }
    else if (error == -EAGAIN)
    {
        LOG_WRN("fetch sample from \"%s\" failed: %d, trying again",
                bme680->name, error);
        goto sample_fetch;
    }
    else
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d",
                bme680->name, error);
    }
}

// Register BME680 sensor callbacks
SensorAPI *register_bme680_callbacks()
{
    LOG_DBG("Registering BME680 callbacks");
    bme680_api.init_sensor = init_sensor;
    bme680_api.read_sensor_values = read_sensor_values;
    bme680_api.data_model_api = register_bme680_model_callbacks();
    return &bme680_api;
}