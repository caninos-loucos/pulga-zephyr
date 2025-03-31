#include <integration/timestamp/timestamp_service.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <drivers/tcs34725.h>
#include <sensors/tcs34725/tcs34725_service.h>

LOG_MODULE_REGISTER(tcs34725_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *tcs34725;
static SensorAPI tcs34725_api = {0};

/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing TCS34725");
    tcs34725 = DEVICE_DT_GET_ANY(ams_tcs34725);

    // Removes sensor API from registered APIs if cannot start sensor
    if (!tcs34725)
    {
        LOG_ERR("TCS34725 not declared at device tree");
        return -ENODEV;
    }
    else if (!device_is_ready(tcs34725))
    {
        LOG_ERR("device \"%s\" is not ready", tcs34725->name);
        return -EAGAIN;
    }

    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading TCS34725");

    SensorModelTCS34725 tcs34725_model;
    uint32_t tcs34725_data[MAX_32_WORDS];
    int error = 0;

 sample_fetch:
    error = sensor_sample_fetch(tcs34725);

    if (error) LOG_DBG("Error at sample fetch");

    // In production yet. Will implement channels later
    // if (!error)
    // {
    //     sensor_channel_get(tcs34725, SENSOR_CHAN_CO2,
    //                        &tcs34725_model.co2);
    //     sensor_channel_get(tcs34725, SENSOR_CHAN_AMBIENT_TEMP,
    //                        &tcs34725_model.temperature);
    //     sensor_channel_get(tcs34725, SENSOR_CHAN_HUMIDITY,
    //                        &tcs34725_model.humidity);

    //     memcpy(&tcs34725_data, &tcs34725_model, sizeof(SensorModelTCS34725));

    //     if (insert_in_buffer(&app_buffer, tcs34725_data, TCS34725_MODEL, error, TCS34725_MODEL_WORDS) != 0)
    //     {
    //         LOG_ERR("Failed to insert data in ring buffer.");
    //     }
    // }
    // else if (error == -EAGAIN)
    // {
    //     LOG_WRN("fetch sample from \"%s\" failed: %d, trying again",
    //             tcs34725->name, error);
    //     goto sample_fetch;
    // }
    // else
    //     LOG_ERR("fetch sample from \"%s\" failed: %d",
    //             tcs34725->name, error);
}

// Register TCS34725 sensor callbacks
SensorAPI *register_tcs34725_callbacks()
{
    LOG_DBG("Registering TCS34725 callbacks");
    tcs34725_api.init_sensor = init_sensor;
    tcs34725_api.read_sensor_values = read_sensor_values;
    tcs34725_api.data_model_api = register_tcs34725_model_callbacks();
    return &tcs34725_api;
}