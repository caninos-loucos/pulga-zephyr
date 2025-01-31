#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <drivers/si1133.h>
#include <sensors/si1133/si1133_service.h>
#include <assert.h>

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
    struct sensor_value val;
    int error;

    assert(sizeof(si1133_model) <= (SI1133_MODEL_WORDS * 4));

    error = sensor_sample_fetch(si1133);
    if (error)
    {
        LOG_ERR("sensor_sample_fetch failed with error %d", error);
        return;
    }

    error = sensor_channel_get(si1133, SENSOR_CHAN_LIGHT,
                               &val);
    if (error)
        goto channel_get_err;

    si1133_model.light = val.val1;

    error = sensor_channel_get(si1133, SENSOR_CHAN_IR,
                               &val);
    if (error)
        goto channel_get_err;

    si1133_model.infrared = val.val1;

    error = sensor_channel_get(si1133, SENSOR_CHAN_UV,
                               &val);
    if (error)
        goto channel_get_err;

    si1133_model.uv = val.val1;

    error = sensor_channel_get(si1133, SENSOR_CHAN_UVI,
                               &val);
    if (error)
        goto channel_get_err;

    si1133_model.uv_index = (val.val1 * 100) + (val.val2 / 10000);

    // Si1133 model is already aligned to 32-bit words, no need for intermediate pointer
    error = insert_in_buffer((uint32_t *)&si1133_model, SI1133_MODEL, error);
    if (error)
        LOG_ERR("Failed to insert data in ring buffer with error %d.", error);
    return;

channel_get_err:
    LOG_ERR("sensor_channel_get failed with error %d", error);
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