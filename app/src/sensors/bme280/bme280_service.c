#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <sensors/bme280/bme280_service.h>
#include <assert.h>

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
    struct sensor_value val;
    int error;
    void *bme280_data;

    assert(sizeof(bme280_model) <= (BME280_MODEL_WORDS * 4));

sample_fetch:
    error = sensor_sample_fetch(bme280);
    if (error == -EAGAIN)
    {
        LOG_WRN("sensor_sample_fetch failed, trying again", error);
        goto sample_fetch;
    }
    else if (error)
    {
        LOG_ERR("sensor_sample_fetch failed with error %d", error);
        return;
    }

    error = sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP,
                               &val);
    if (error)
        goto channel_get_err;

    // Temperature in centidegrees ( 1/100th of one degree Celsius )
    bme280_model.temperature = val.val1 * 100 + val.val2 / 10000;

    error = sensor_channel_get(bme280, SENSOR_CHAN_PRESS,
                               &val);
    if (error)
        goto channel_get_err;

    // Pressure in hPa
    bme280_model.pressure = val.val1 * 100 + val.val2 / 10000;

    error = sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY,
                               &val);
    if (error)
        goto channel_get_err;

    // Humidity in %RH
    bme280_model.humidity = val.val1;

    bme280_data = calloc(BME280_MODEL_WORDS, 4);
    if (!bme280_data)
    {
        LOG_ERR("could not allocate pointer");
        return;
    }
    memcpy(bme280_data, &bme280_model, sizeof(SensorModelBME280));

    error = insert_in_buffer(bme280_data, BME280_MODEL, error);
    free(bme280_data);

    if (error)
        LOG_ERR("Failed to insert data in ring buffer with error %d.", error);
    return;

channel_get_err:
    LOG_ERR("sensor_channel_get failed with error %d", error);
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