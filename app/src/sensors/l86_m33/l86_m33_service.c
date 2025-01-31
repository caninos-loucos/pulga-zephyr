#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
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

    l86_m33_data = calloc(GNSS_MODEL_WORDS, 4);
    if (!l86_m33_data)
    {
        LOG_ERR("could not allocate pointer");
        return;
    }

    memcpy(l86_m33_data, &gnss_model, sizeof(SensorModelGNSS));
    error = insert_in_buffer(l86_m33_data, GNSS_MODEL, 0);
    free(l86_m33_data);

    if (error)
        LOG_ERR("Failed to insert data in ring buffer with error %d.", error);
}

int set_valid_fix_interval(int raw_fix_interval)
{
    int error, clamped_fix_interval;

    // Device only allows 100 ms to 10000 ms update rate
    clamped_fix_interval = CLAMP(raw_fix_interval, 100, 10000);
    if(clamped_fix_interval != raw_fix_interval)
        LOG_WRN("Invalid fix interval provided (%d), clamping to [100, 10000]ms interval", raw_fix_interval);

    // If it's greater than 1000, fix rate must be a multiple of 1000
    if(clamped_fix_interval > 1000)
        clamped_fix_interval -= clamped_fix_interval % 1000;

    error = gnss_set_fix_rate(l86_m33, clamped_fix_interval);
    if (error)
        LOG_ERR("Couldn't set L86-M33 fix rate");
    
    return error;
}

// Register L86_M33 sensor callbacks
SensorAPI *register_l86_m33_callbacks()
{
    LOG_DBG("Registering L86-M33 callbacks");
    l86_m33_api.init_sensor = init_sensor;
    l86_m33_api.read_sensor_values = read_sensor_values;
    l86_m33_api.data_model_api = register_gnss_model_callbacks();
    return &l86_m33_api;
}