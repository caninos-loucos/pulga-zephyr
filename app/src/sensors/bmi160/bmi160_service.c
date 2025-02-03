#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <sensors/bmi160/bmi160_service.h>
#include <assert.h>

LOG_MODULE_REGISTER(bmi160_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *bmi160;
static SensorAPI bmi160_api = {0};

/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing BMI160");
    bmi160 = DEVICE_DT_GET_ANY(bosch_bmi160);

    // Removes sensor API from registered APIs if cannot start sensor
    if (!bmi160)
    {
        LOG_ERR("bmi160 not declared at device tree");
        return -ENODEV;
    }
    else if (!device_is_ready(bmi160))
    {
        LOG_ERR("device \"%s\" is not ready", bmi160->name);
        return -EAGAIN;
    }
    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading BMI160");

    SensorModelBMI160 bmi160_model;
    struct sensor_value val[3];
    int error, i;

    assert(sizeof(bmi160_model) <= (BMI160_MODEL_WORDS * 4));

sample_fetch:
    error = sensor_sample_fetch(bmi160);
    if (error == -EAGAIN)
    {
        LOG_WRN("sensor_sample_fetch failed with error %d, trying again", error);
        goto sample_fetch;
    }
    else if (error)
    {
        LOG_ERR("sensor_sample_fetch failed with error %d", error);
        return;
    }

    error = sensor_channel_get(bmi160, SENSOR_CHAN_ACCEL_XYZ,
                               val);
    if (error)
        goto channel_get_err;

    // Acceleration in cm/s
    for (i = 0; i < 3; i++)
        bmi160_model.acceleration[i] = val[i].val1 * 100 + val[i].val2 / 10000;

    error = sensor_channel_get(bmi160, SENSOR_CHAN_GYRO_XYZ,
                               val);
    if (error)
        goto channel_get_err;

    // Rotation in milliradians/s
    for (i = 0; i < 3; i++)
        bmi160_model.rotation[i] = val[i].val1 * 1000 + val[i].val2 / 1000;

    // BMI160 is already aligned to 32-bit so there's no need to copy it to a temp pointer
    error = insert_in_buffer((uint32_t *)&bmi160_model, BMI160_MODEL, error);
    if (error)
        LOG_ERR("Failed to insert data in ring buffer with error %d.", error);
    return;

channel_get_err:
    LOG_ERR("sensor_channel_get failed with error %d", error);
}

// Register BMI160 sensor callbacks
SensorAPI *register_bmi160_callbacks()
{
    LOG_DBG("Registering BMI160 callbacks");
    bmi160_api.init_sensor = init_sensor;
    bmi160_api.read_sensor_values = read_sensor_values;
    bmi160_api.data_model_api = register_bmi160_model_callbacks();
    return &bmi160_api;
}