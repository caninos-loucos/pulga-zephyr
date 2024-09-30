#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <sensors/scd30/scd30_service.h>

LOG_MODULE_REGISTER(scd30_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *scd30;
static SensorAPI scd30_api = {0};


/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing SCD30");
    scd30 = DEVICE_DT_GET_ANY(sensirion_scd30);

    // Removes sensor API from registered APIs if cannot start sensor
    if (!scd30)
    {
        LOG_ERR("SDC30 not declared at device tree");
        return -1;
    }
    else if (!device_is_ready(scd30))
    {
        LOG_ERR("device \"%s\" is not ready", scd30->name);
        return -2;
    }

    struct sensor_value *period = {0};

    period->val1 = get_sampling_interval();

    // Will set a new sample period from here
    // SENSOR_ATTR_PRIV_START is a provisory solution!
    sensor_attr_set(scd30, SENSOR_CHAN_ALL, SENSOR_ATTR_PRIV_START, period);
    
    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading SCD30");

    SensorModelSCD30 scd30_model;
    uint32_t scd30_data[MAX_32_WORDS];
    int error = 0;

    error = sensor_sample_fetch(scd30);
    if (!error)
    {
        sensor_channel_get(scd30, SENSOR_CHAN_CO2,
						   &scd30_model.co2);
		sensor_channel_get(scd30, SENSOR_CHAN_AMBIENT_TEMP,
						   &scd30_model.temperature);
		sensor_channel_get(scd30, SENSOR_CHAN_HUMIDITY,
						   &scd30_model.humidity);
    }
    else
    {
        LOG_ERR("fetch sample from \"%s\" failed: %d",
                "SCD30", error);
    }

    memcpy(&scd30_data, &scd30_model, sizeof(SensorModelSCD30));

    if (insert_in_buffer(scd30_data, SCD30_MODEL, error) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
    }
}

// Register SCD30 sensor callbacks
SensorAPI *register_scd30_callbacks()
{
    LOG_DBG("Registering SCD30 callbacks");
    scd30_api.init_sensor = init_sensor;
    scd30_api.read_sensor_values = read_sensor_values;
    scd30_api.data_model_api = register_scd30_model_callbacks();
    return &scd30_api;
}