#include <zephyr/logging/log.h>
#include <sensors/vbatt/vbatt_service.h>
#include <integration/timestamp/timestamp_service.h>

LOG_MODULE_REGISTER(vbatt_service, CONFIG_APP_LOG_LEVEL);

#define VBATT DT_PATH(vbatt)

/**
 * DEFINITIONS
 */

static const struct device *divider;
static SensorAPI vbatt_api = {0};

static int init_sensor(void);
static void read_sensor_values(void);

void warn_low_battery(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(low_battery_work, warn_low_battery);

/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing vbatt");

    divider = DEVICE_DT_GET(VBATT);
    if (!divider)
    {
        LOG_ERR("\"vbatt\" not declared at device tree");
        return -ENODEV;
    }
    else if (!device_is_ready(divider))
    {
        LOG_ERR("device \"%s\" is not ready", divider->name);
        return -EAGAIN;
    }

    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading %s", divider->name);

    SensorModelVbatt vbatt_model;
    uint32_t vbatt_data[MAX_32_WORDS];

    // Fetch measurements to driver
    int error = sensor_sample_fetch(divider);
    while (error == -EAGAIN)
    {
        LOG_WRN("fetch sample from \"%s\" failed: %d, trying again", divider->name, error);
        error = sensor_sample_fetch(divider);
    }
    if (error)
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d", divider->name, error);
        return;
    }

    // Get voltage from driver
    error = sensor_channel_get(divider, SENSOR_CHAN_VOLTAGE, &vbatt_model.voltage);
    if (error)
    {
        LOG_ERR("getting sample from \"%s\" failed: %d", divider->name, error);
        return;
    }

#ifndef CONFIG_EVENT_TIMESTAMP_NONE
    vbatt_model.timestamp = get_current_timestamp();
#endif /* CONFIG_EVENT_TIMESTAMP_NONE */

    // Load battery data to application buffer
    memcpy(&vbatt_data, &vbatt_model, sizeof(SensorModelVbatt));
    if (insert_in_buffer(&app_buffer, vbatt_data, VBATT_MODEL, 0, VBATT_MODEL_WORDS) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
    }

    // Schedule low-battery trigger
    if (sensor_value_to_milli(&vbatt_model.voltage) < CONFIG_LOW_BATT_THRESH)
    {
        LOG_WRN("Battery low: %d mV", (int16_t)sensor_value_to_milli(&vbatt_model.voltage));
        k_work_schedule(&low_battery_work, K_MSEC(2000));
    }
}

void warn_low_battery(struct k_work *work)
{
    LOG_WRN("Battery below %d.%03d V",
            CONFIG_LOW_BATT_THRESH / 1000,
            CONFIG_LOW_BATT_THRESH % 1000);
}

// Register vbatt sensor callbacks
SensorAPI *register_vbatt_callbacks()
{
    LOG_DBG("Registering vbatt callbacks");
    vbatt_api.init_sensor = init_sensor;
    vbatt_api.read_sensor_values = read_sensor_values;
    vbatt_api.data_model_api = register_vbatt_model_callbacks();
    return &vbatt_api;
}