#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>
#include <integration/timestamp/timestamp_service.h>
#include <sensors/l86_m33/l86_m33_service.h>
#include <stdlib.h>
#include <assert.h>

LOG_MODULE_REGISTER(l86_m33_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

const struct device *const l86_m33 = DEVICE_DT_GET(DT_ALIAS(gnss));
// API that concentrates the methods to deal with GNSS (GPS) module
static SensorAPI l86_m33_api = {0};
// Semaphore that allows for the received data to be inserted in the buffer
struct k_sem process_fix_data;

// Handles the incoming reading from the satellites, but only saves to
// buffer according to overall sampling time
void receive_fix_callback(const struct device *gnss_device,
                          const struct gnss_data *gnss_data);
// Sets the fix interval of GNSS model to a valid number according to device's restrictions
int set_valid_fix_interval(int raw_fix_interval);
// Rounds the value to the closest multiple of 1000
static int round_closest_1000_multiple(int number);
#if defined(CONFIG_EVENT_TIMESTAMP_GNSS)
// Converts the GNSS time to a timestamp and sets it as the synchronization time
static void convert_and_set_sync_time(const struct gnss_data *gnss_data);
#endif
/**
 * IMPLEMENTATIONS
 */

// Gets and initializes device
static int init_sensor()
{
    LOG_DBG("Initializing L86-M33 GNSS module");
    int error = 0;
    // Removes sensor API from registered APIs if cannot start sensor
    if (!l86_m33)
    {
        LOG_ERR("l86_m33 not declared at device tree");
        return -ENODEV;
    }
    error = device_init(l86_m33);
    if (error)
    {
        LOG_ERR("Device \"%s\" could not be initialized", l86_m33->name);
        return error;
    }
    error = set_valid_fix_interval(get_sampling_interval());
    if (error)
    {
        return error;
    }
    error = k_sem_init(&process_fix_data, 0, 1);
    if (error)
    {
        LOG_ERR("Failed to initialize GNSS semaphore: %d", error);
        return error;
    }
    GNSS_DATA_CALLBACK_DEFINE(l86_m33, receive_fix_callback);

    return 0;
}

// Reads sensor measurements and stores them in buffer
static void read_sensor_values()
{
    LOG_DBG("Allowing L86-M33 to store fix data in buffer");
    k_sem_give(&process_fix_data);
}

void receive_fix_callback(const struct device *gnss_device,
                          const struct gnss_data *gnss_data)
{
    SensorModelGNSS gnss_model;
    void *l86_m33_data;
    int error;

    assert(sizeof(gnss_model) <= (GNSS_MODEL_WORDS * 4));

    gnss_model.dataType = (uint8_t) GNSS_MODEL;

    // Only saves to buffer if it's a valid fix, it can take several minutes
    // to start gettting valid readings from the satellites
    if (gnss_data->info.fix_status == GNSS_FIX_STATUS_NO_FIX)
        return;

    // Returns if it's not supposed to save data to buffer
    if (k_sem_take(&process_fix_data, K_NO_WAIT))
        return;

    // Latitude and Longitude in microdegrees (-180E6 to 180E6)
    gnss_model.latitude = gnss_data->nav_data.latitude / 1000;
    gnss_model.longitude = gnss_data->nav_data.longitude / 1000;

    // Altitude in decimeters (1/10th of meter)
    gnss_model.altitude = gnss_data->nav_data.altitude / 10;

    // Speed in cm/s
    gnss_model.speed = gnss_data->nav_data.speed / 10;

    // Bearing angle in decidegrees (1/10th of a degree)
    gnss_model.bearing = gnss_data->nav_data.bearing / 10;

    gnss_model.real_time = gnss_data->utc;

#ifndef CONFIG_EVENT_TIMESTAMP_NONE
    gnss_model.timestamp = get_current_timestamp();
#endif /* CONFIG_EVENT_TIMESTAMP_NONE */

    if (insert_in_buffer(&app_buffer, (uint32_t *)&gnss_model, GNSS_MODEL, 0, GNSS_MODEL_WORDS) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
    }
}

int set_valid_fix_interval(int raw_fix_interval)
{
    int error, clamped_fix_interval;

    // Device only allows 100 ms to 10000 ms update rate
    clamped_fix_interval = CLAMP(raw_fix_interval, 100, 10000);
    // If it's greater than 1000, fix rate must be a multiple of 1000
    if (clamped_fix_interval > 1000)
        clamped_fix_interval -= clamped_fix_interval % 1000;

    if (clamped_fix_interval != raw_fix_interval)
        LOG_WRN("Invalid fix interval provided (%d), clamping to [100, 10000]ms interval (%d)", raw_fix_interval, clamped_fix_interval);

    error = gnss_set_fix_rate(l86_m33, clamped_fix_interval);
    if (error)
        LOG_ERR("Couldn't set L86-M33 fix rate");

    return error;
}

int round_closest_1000_multiple(int number)
{
    int multiple = 1000;

    int floor = (number / multiple) * multiple;
    int ceiling = floor + multiple;

    // Closest
    return (number - floor >= ceiling - number) ? ceiling : floor;
}

#if defined(CONFIG_EVENT_TIMESTAMP_GNSS)
static void convert_and_set_sync_time(const struct gnss_data *gnss_data)
{
    // Converts GNSS time to timestamp
    struct tm structured_time = {
        .tm_sec = gnss_data->utc.millisecond / 1000,
        .tm_min = gnss_data->utc.minute,
        .tm_hour = gnss_data->utc.hour,
        .tm_mday = gnss_data->utc.month_day,
        .tm_mon = gnss_data->utc.month - 1,
        .tm_year = gnss_data->utc.century_year + 2000 - TIME_UTILS_BASE_YEAR,
    };
    uint64_t gps_epoch = timeutil_timegm64(&structured_time);
    LOG_INF("GNSS time: %lld", gps_epoch);
    // Sets the timestamp as the synchronization time
    set_sync_time_seconds(gps_epoch);
}
#endif

// Register L86_M33 sensor callbacks
SensorAPI *register_l86_m33_callbacks()
{
    LOG_DBG("Registering L86-M33 callbacks");
    l86_m33_api.init_sensor = init_sensor;
    l86_m33_api.read_sensor_values = read_sensor_values;
    l86_m33_api.data_model_api = register_gnss_model_callbacks();
    return &l86_m33_api;
}