#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <drivers/si1133.h>
#include <sensors/si1133/si1133_service.h>

LOG_MODULE_REGISTER(si1133_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *si1133;
static SensorAPI si1133_api = {0};

/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing Si1133");
    si1133 = DEVICE_DT_GET_ANY(silabs_si1133);

    // Removes sensor API from registered APIs if cannot start sensor
    if (!si1133)
    {
        LOG_ERR("si1133 not declared at device tree");
        return -ENODEV;
    }
    else if (!device_is_ready(si1133))
    {
        LOG_ERR("device \"%s\" is not ready", si1133->name);
        return -EAGAIN;
    }
    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading Si1133");

    SensorModelSi1133 si1133_model;
    uint32_t si1133_data[MAX_32_WORDS];
    int error = 0;

sample_fetch:
    error = sensor_sample_fetch(si1133);
    if (!error)
    {
        sensor_channel_get(si1133, SENSOR_CHAN_LIGHT,
                           &si1133_model.light);
        sensor_channel_get(si1133, SENSOR_CHAN_IR,
                           &si1133_model.infrared);
        sensor_channel_get(si1133, SENSOR_CHAN_UV,
                           &si1133_model.uv);
        sensor_channel_get(si1133, SENSOR_CHAN_UVI,
                           &si1133_model.uv_index);

        memcpy(&si1133_data, &si1133_model, sizeof(SensorModelSi1133));

        if (insert_in_buffer(si1133_data, SI1133_MODEL, error) != 0)
        {
            LOG_ERR("Failed to insert data in ring buffer.");
        }
    }
    else if (error == -EAGAIN)
    {
        LOG_WRN("fetch sample from \"%s\" failed: %d, trying again",
                si1133->name, error);
        goto sample_fetch;
    }
    else
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d",
                si1133->name, error);
    }
}

// Register Si1133 sensor callbacks
SensorAPI *register_si1133_callbacks()
{
    LOG_DBG("Registering Si1133 callbacks");
    si1133_api.init_sensor = init_sensor;
    si1133_api.read_sensor_values = read_sensor_values;
    si1133_api.data_model_api = register_si1133_model_callbacks();
    return &si1133_api;
}