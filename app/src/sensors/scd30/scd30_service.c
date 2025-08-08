#include <integration/timestamp/timestamp_service.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <drivers/scd30.h>
#include <sensors/scd30/scd30_service.h>

LOG_MODULE_REGISTER(scd30_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

/** Response time of 30s to read 63% of the value after a sudden change in CO2 concentration,
 *  considering sampling period of 5s after calibration. Refer to driver and datasheets
 *  for more details. Change this value to adapt to different sampling periods or to debug
 *  the application faster.
 */
#define SCD30_RESPONSE_TIME K_SECONDS(30)
static const struct device *scd30;
static SensorAPI scd30_api = {0};
// Semaphore to synchronize access to the buffer for storing sensor data
static struct k_sem store_data;
/**
 * This function allows storing data from the SCD30 sensor into the application buffer
 * after the sensor has stabilized, considering its response time after starting
 * periodic measurement.
 */
static inline void store_stabilized_data(struct k_work *work);
/**
 * Work item for delaying the storage of sensor data until after the sensor has stabilized.
 */
K_WORK_DELAYABLE_DEFINE(trigger_stabilized_sensor_routine, store_stabilized_data);
/**
 * @brief Callback function to read sensor data from device instance.
 *>
 * This function is triggered when the SCD30 sensor has new data available.
 * It first checks if it is allowed to save data by attempting to take the semaphore that
 * signals permission to store data after stabilization of the sensor.
 * If the semaphore is not available, the function returns immediately.
 *
 * The function retrieves CO2, temperature, and humidity data from the SCD30 sensor,
 * stores it in a SensorModelSCD30 structure, and then copies this data into the aaplication
 * buffer. After that, it stops the periodic measurement to save power.
 *
 * If the data insertion into the buffer fails, an error message is logged.
 */
static void read_data_callback();
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
        return -ENODEV;
    }
    else if (!device_is_ready(scd30))
    {
        LOG_ERR("device \"%s\" is not ready", scd30->name);
        return -EAGAIN;
    }
    error = k_sem_init(&store_data, 0, 1);
    if (error)
    {
        LOG_ERR("Failed to initialize SCD30 semaphore: %d", error);
        return error;
    }

    // Starts periodic measurements with default ambient pressure if not already started
    error = scd30_start_periodic_measurement(scd30, SCD30_SAO_PAULO_AMBIENT_PRESSURE);

    // Registers desired application callback into the scd30 driver api
    scd30_register_callback(scd30, read_data_callback);

    // Warns the sampling interval isn't enough for stabilization
    if (get_sampling_interval() < k_ticks_to_ms_floor32(SCD30_RESPONSE_TIME.ticks))
    {
        LOG_WRN("Sampling interval is less than SCD30 response time. Data will "
                "be reliable after %d seconds.",
                k_ticks_to_ms_floor32(SCD30_RESPONSE_TIME.ticks) / MSEC_PER_SEC);
    }

    return error;
}

inline void store_stabilized_data(struct k_work *work)
{
    ARG_UNUSED(work);
    // Allows storing data from SCD30 sensor
    k_sem_give(&store_data);
}

void read_data_callback()
{
    // Returns if it's not supposed to save data to buffer
    if (k_sem_take(&store_data, K_NO_WAIT))
    {
        return;
    }

    LOG_DBG("Storing SCD30 data");

    SensorModelSCD30 scd30_model = {0};
    uint32_t scd30_data[MAX_32_WORDS];
    int error = 0;

    sensor_channel_get(scd30, SENSOR_CHAN_CO2,
                       &scd30_model.co2);
    sensor_channel_get(scd30, SENSOR_CHAN_AMBIENT_TEMP,
                       &scd30_model.temperature);
    sensor_channel_get(scd30, SENSOR_CHAN_HUMIDITY,
                       &scd30_model.humidity);
#ifndef CONFIG_EVENT_TIMESTAMP_NONE
    scd30_model.timestamp = get_current_timestamp();
#endif /* CONFIG_EVENT_TIMESTAMP_NONE */
    memcpy(&scd30_data, &scd30_model, sizeof(SensorModelSCD30));

    if (insert_in_buffer(&app_buffer, scd30_data, SCD30_MODEL, error, SCD30_MODEL_WORDS) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
    }

    if (get_sampling_interval() >= k_ticks_to_ms_floor32(SCD30_RESPONSE_TIME.ticks))
    {
        // Stops periodic measurement to save power
        scd30_stop_periodic_measurement(scd30);
    }
}

static inline void read_sensor_values()
{

    if (get_sampling_interval() < k_ticks_to_ms_floor32(SCD30_RESPONSE_TIME.ticks))
    {
        k_work_schedule(&trigger_stabilized_sensor_routine, K_NO_WAIT);
    }
    else
    {
        LOG_DBG("Waking up SCD30 to read data");
        // Waking up SCD30 to read data
        scd30_start_periodic_measurement(scd30, SCD30_SAO_PAULO_AMBIENT_PRESSURE);
        // Scheduling data storage after sensor response time
        k_work_schedule(&trigger_stabilized_sensor_routine, SCD30_RESPONSE_TIME);
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