#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <drivers/scd30.h>
#include <sensors/scd30/scd30_service.h>
<<<<<<< HEAD
#include <assert.h>
=======
>>>>>>> origin/main

LOG_MODULE_REGISTER(scd30_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *scd30;
static SensorAPI scd30_api = {0};

// Sets the sample time of the aplication into the SCD30 device,
// clipping it since it only allows measurement time from 2s to 1800s.
// raw_sample_time is given in ms
int set_valid_sample_time(int raw_sample_time);

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
        return -ENODEV;
    }
    else if (!device_is_ready(scd30))
    {
        LOG_ERR("device \"%s\" is not ready", scd30->name);
        return -EAGAIN;
    }

    // Try to set the application sampling time
    set_valid_sample_time(get_sampling_interval());

    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Reading SCD30");

    SensorModelSCD30 scd30_model;
    int error;
    struct sensor_value val;
    void *scd30_data;

    assert(sizeof(scd30_model) <= (SCD30_MODEL_WORDS * 4));

sample_fetch:
    error = sensor_sample_fetch(scd30);

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

    error = sensor_channel_get(scd30, SENSOR_CHAN_CO2, &val);
    if (error)
        goto channel_get_err;

    // CO2 range (with guaranteed accuracy):  0-10000 ppm
    scd30_model.co2 = val.val1;

    error = sensor_channel_get(scd30, SENSOR_CHAN_AMBIENT_TEMP, &val);
    if (error)
        goto channel_get_err;

    // Temperature is stored in centidegrees ( 1/100th of a degree Celsius )
    scd30_model.temperature = val.val1 * 100 + val.val2 / 10000;

    error = sensor_channel_get(scd30, SENSOR_CHAN_HUMIDITY, &val);
    if (error)
        goto channel_get_err;

    // Humidity stored in %RH
    scd30_model.humidity = val.val1;

    scd30_data = calloc(SCD30_MODEL_WORDS, 4);
    if (!scd30_data)
    {
        LOG_ERR("could not to allocate pointer");
        return;
    }

    memcpy(scd30_data, &scd30_model, sizeof(scd30_model));
    error = insert_in_buffer(&app_buffer, (uint32_t*)&scd30_data, SCD30_MODEL, error, SCD30_MODEL_WORDS);
    free(scd30_data);

    if (error)
        LOG_ERR("Failed to insert data in ring buffer with error %d.", error);

    return;

channel_get_err:
    LOG_ERR("sensor_channel_get failed with error %d", error);
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

int set_valid_sample_time(int raw_sample_time)
{
    int error = 0;
    struct sensor_value period;

    raw_sample_time /= 1000;
    period.val1 = CLAMP(raw_sample_time, 2, 1800);

    if (period.val1 != raw_sample_time)
    {
        LOG_INF("Samplig period outside SCD30 specification, SCD30 set to sample every %d seconds.",
                period.val1);
    }

    // Send the chosen period to the driver
    error = sensor_attr_set(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_SAMPLING_PERIOD, &period);
    if (error)
    {
        LOG_ERR("Could not set application sample time. Error code: %d", error);
        return error;
    }

    return 0;
}