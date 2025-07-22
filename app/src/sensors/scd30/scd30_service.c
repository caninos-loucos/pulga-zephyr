#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <drivers/scd30.h>
#include <sensors/scd30/scd30_service.h>
#include <math.h>

LOG_MODULE_REGISTER(scd30_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static const struct device *scd30;
static SensorAPI scd30_api = {0};
// Semaphore to synchronize access to the buffer for storing sensor data
static struct k_sem store_data;

/**
 * Sets a valid sample time for the SCD30 sensor.
 * Clips the sample time to be within the allowed range of 2s to 1800s, as
 * specified on the datasheet.
 *
 * @param raw_sample_time The desired sample time in milliseconds.
 * @return 0 on success, or a negative error code on failure.
 */
static inline int set_valid_sample_time(int raw_sample_time);
/**
 * @brief Callback function to store sensor data.
 *
 * This function is triggered to store data from the SCD30 sensor into a buffer.
 * It first checks if it is allowed to save data by attempting to take a semaphore.
 * If the semaphore is not available, the function returns immediately.
 *
 * The function retrieves CO2, temperature, and humidity data from the SCD30 sensor,
 * stores it in a SensorModelSCD30 structure, and then copies this data into a buffer.
 *
 * If the data insertion into the buffer fails, an error message is logged.
 */
static void store_data_callback();
/**
 * @brief Reads sensor values from the SCD30 sensor and stores them in a buffer.
 */
static inline void read_sensor_values();

/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing SCD30");
    scd30 = DEVICE_DT_GET_ANY(sensirion_scd30);
    int error = 0;

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
    error = k_sem_init(&store_data, 0, 1);
    if (error)
    {
        LOG_ERR("Failed to initialize SCD30 semaphore: %d", error);
        return error;
    }

    // Registers desired application callback into the scd30 driver api
    scd30_register_callback(scd30, store_data_callback);

    // Try to set the application sampling time
    set_valid_sample_time(get_sampling_interval());

    return 0;
}

inline int set_valid_sample_time(int raw_sample_time)
{
    int error = 0;
    struct sensor_value period;

    raw_sample_time /= 1000;

    // Clip the value using mathemagical properties
    period.val1 = (int)fmax(2, fmin(raw_sample_time, 1800));
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

void store_data_callback()
{

    // Returns if it's not supposed to save data to buffer
    if (k_sem_take(&store_data, K_NO_WAIT))
    {
        return;
    }

    LOG_DBG("Storing SCD30 data");

    SensorModelSCD30 scd30_model;
    uint32_t scd30_data[MAX_32_WORDS];
    int error = 0;

    sensor_channel_get(scd30, SENSOR_CHAN_CO2,
                       &scd30_model.co2);
    sensor_channel_get(scd30, SENSOR_CHAN_AMBIENT_TEMP,
                       &scd30_model.temperature);
    sensor_channel_get(scd30, SENSOR_CHAN_HUMIDITY,
                       &scd30_model.humidity);
    memcpy(&scd30_data, &scd30_model, sizeof(SensorModelSCD30));

    if (insert_in_buffer(scd30_data, SCD30_MODEL, error) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
    }
}

static inline void read_sensor_values()
{
    LOG_DBG("Allowing SCD30 to store fix data in buffer");
    k_sem_give(&store_data);

    /**
     *  Note: Uncomment code bellow if you want the application to perform consecutive readings.
     *  Otherwise, it will wait the interrupt routine in driver, associated with the sampling
     *  rate configured in the sensor.
     */

    // LOG_DBG("Reading SCD30");

    // SensorModelSCD30 scd30_model;
    // uint32_t scd30_data[MAX_32_WORDS];
    // int error = 0;

    // error = sensor_sample_fetch(scd30);
    // if (!error)
    // {
    //     k_sem_give(&store_data);
    //     store_data_callback();
    // }
    // else
    //     LOG_ERR("fetch_sample failed: %d", error);
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