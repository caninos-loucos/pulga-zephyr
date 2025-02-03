#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <sensors/bme280/bme280_service.h>

LOG_MODULE_REGISTER(bme280_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *bme280;
static SensorAPI bme280_api = {0};

/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing BME280");
    bme280 = DEVICE_DT_GET_ANY(bosch_bme280);

    // Removes sensor API from registered APIs if cannot start sensor
    if (!bme280)
    {
        LOG_ERR("bme280 not declared at device tree");
        return -ENODEV;
    }
    else if (!device_is_ready(bme280))
    {
        LOG_ERR("device \"%s\" is not ready", bme280->name);
        return -EAGAIN;
    }
    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading BME280");

    SensorModelBME280 bme280_model;
    uint32_t bme280_data[MAX_32_WORDS];
    int error;

sample_fetch:
    error = sensor_sample_fetch(bme280);
    if (!error)
    {
        sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP,
                           &bme280_model.temperature);
        sensor_channel_get(bme280, SENSOR_CHAN_PRESS,
                           &bme280_model.pressure);
        sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY,
                           &bme280_model.humidity);

        memcpy(&bme280_data, &bme280_model, sizeof(SensorModelBME280));

        if (insert_in_buffer(bme280_data, BME280_MODEL, error) != 0)
        {
            LOG_ERR("Failed to insert data in ring buffer.");
        }
    }
    else if (error == -EAGAIN)
    {
        LOG_WRN("fetch sample from \"%s\" failed: %d, trying again",
                bme280->name, error);
        goto sample_fetch;
    }
    else
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d",
                bme280->name, error);
    }
}

// Register BME280 sensor callbacks
SensorAPI *register_bme280_callbacks()
{
    LOG_DBG("Registering BME280 callbacks");
    bme280_api.init_sensor = init_sensor;
    bme280_api.read_sensor_values = read_sensor_values;
    bme280_api.data_model_api = register_bme280_model_callbacks();
    return &bme280_api;
}