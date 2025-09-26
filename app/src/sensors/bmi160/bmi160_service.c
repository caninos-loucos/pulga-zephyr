#include <integration/timestamp/timestamp_service.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <sensors/bmi160/bmi160_service.h>

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
    uint32_t bmi160_data[MAX_32_WORDS];
    int error = 0;

sample_fetch:
    error = sensor_sample_fetch(bmi160);
    if (!error)
    {
        sensor_channel_get(bmi160, SENSOR_CHAN_ACCEL_XYZ,
                           bmi160_model.acceleration);
        sensor_channel_get(bmi160, SENSOR_CHAN_GYRO_XYZ,
                           bmi160_model.rotation);
#ifndef CONFIG_EVENT_TIMESTAMP_NONE
        bmi160_model.timestamp = get_current_timestamp();
#endif /* CONFIG_EVENT_TIMESTAMP_NONE */

        memcpy(&bmi160_data, &bmi160_model, sizeof(SensorModelBMI160));

        if (insert_in_buffer(&app_buffer, bmi160_data, BMI160_MODEL, error, BMI160_MODEL_WORDS) != 0)
        {
            LOG_ERR("Failed to insert BMI160 data in ring buffer.");
        }
    }
    else if (error == -EAGAIN)
    {
        LOG_WRN("fetch sample from \"%s\" failed: %d, trying again",
                bmi160->name, error);
        goto sample_fetch;
    }
    else
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d",
                bmi160->name, error);
    }
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