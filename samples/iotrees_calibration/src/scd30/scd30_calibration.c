#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/scd30.h>
#include <scd30/scd30_calibration.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(scd30_calibration, CONFIG_CALIBRATION_LOG_LEVEL);

/**
 * DEFINITIONS
 */
/**
 * @brief SCD30 sensor device instance.
 */
static const struct device *scd30;
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
 * @brief This function is triggered to present data from the SCD30 sensor.
 */
static inline void present_data_callback();
/**
 * Disables the automatic self-calibration feature of the SCD30 sensor.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline int disable_auto_calibration();
/**
 * Enables SCD30 forced recalibration.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline void force_calibration(struct k_work *work);
// /**
//  * Sets the temperature offset for the SCD30 sensor.
//  *
//  * @return 0 on success, or a negative error code on failure.
//  */
// static inline void set_temperature_offset(struct k_work *work);
/**
 * Work item for deferring the forced recalibration of the SCD30 sensor until 
 * it warms up (2 minutes or 5 times the sampling rate).
 */
K_WORK_DELAYABLE_DEFINE(trigger_forced_calibration_work, force_calibration);
// /**
//  * Work item for deferring the setting of the temperature offset for the SCD30 sensor.
//  */
// K_WORK_DELAYABLE_DEFINE(trigger_temperature_offset_work, set_temperature_offset);

/**
 * IMPLEMENTATIONS
 */

int init_scd30()
{
    LOG_DBG("Initializing SCD30");
    scd30 = DEVICE_DT_GET_ANY(sensirion_scd30);

    if (!scd30)
    {
        LOG_ERR("SDC30 not declared at device tree");
        return -ENODEV;
    }
    else if (!device_is_ready(scd30))
    {
        LOG_ERR("SCD30 is not ready");
        return -EAGAIN;
    }
    // Registers desired application callback into the scd30 driver api
    scd30_register_callback(scd30, present_data_callback);

    return 0;
}

int enable_scd30_low_power_mode()
{
    int error = 0;

    // Disable automatic self-calibration
    error = disable_auto_calibration();
    if (error)
    {
        LOG_ERR("Failed to disable auto calibration: %d", error);
        return error;
    }

    // Set the sample time
    error = set_valid_sample_time(SCD30_SAMPLING_RATE);
    if (error)
    {
        LOG_ERR("Failed to set sample time: %d", error);
        return error;
    }

    // Schedule the forced recalibration after 5 times the sampling rate
    int calibration_delay = SCD30_SAMPLING_RATE * 5;
    calibration_delay = MAX(calibration_delay, 120000); // Ensure at least 10 seconds
    k_work_schedule(&trigger_forced_calibration_work, K_MSEC(calibration_delay));
    // Schedule the temperature offset setting after 10 

    return 0;
}

static inline int disable_auto_calibration()
{
    int error = 0;
    struct sensor_value auto_calibration;

    do
    {
        // Checking if automatic self-calibration is enabled
        error = sensor_attr_get(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_AUTO_SELF_CALIBRATION,
                                &auto_calibration);
        if (error != 0)
        {
            LOG_ERR("Failed to get auto self calibration status: %d", error);
            return error;
        }
        if (auto_calibration.val1 == 0)
        {
            LOG_INF("SCD30 auto calibration disabled");
            return 0;
        }

        LOG_INF("SCD30 auto calibration enabled, disabling it now...");
        error = sensor_attr_set(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_AUTO_SELF_CALIBRATION, &auto_calibration);
        if (error)
        {
            LOG_ERR("Could not disable auto calibration. Error code: %d", error);
            return error;
        }
    } while (1);

    return 0;
}

static inline int set_valid_sample_time(int raw_sample_time)
{
    int error = 0;
    struct sensor_value period;

    raw_sample_time /= 1000;

    // Clip the value using mathemagical properties
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

    LOG_DBG("SCD30 sample time set to %d seconds", period.val1);

    return 0;
}

static inline void force_calibration(struct k_work *work)
{
    int error = 0;
    struct sensor_value calibration_reference;
    LOG_DBG("Forcing SCD30 calibration...");

    do
    {
        // Checking if CO2 reference is set to the right value
        error = sensor_attr_get(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_FORCED_RECALIBRATION,
                                &calibration_reference);
        if (error != 0)
        {
            LOG_ERR("Failed to get CO2 reference value: %d", error);
            k_work_schedule(&trigger_forced_calibration_work, K_NO_WAIT);
        }
        if (calibration_reference.val1 == SCD30_CO2_REFERENCE)
        {
            LOG_INF("SCD30 CO2 reference set to %d ppm",
                    SCD30_CO2_REFERENCE);
            return;
        }

        LOG_INF("Setting SCD30 CO2 reference to %d ppm", SCD30_CO2_REFERENCE);
        calibration_reference.val1 = SCD30_CO2_REFERENCE;
        error = sensor_attr_set(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_FORCED_RECALIBRATION,
             &calibration_reference);
        if (error)
        {
            LOG_ERR("Could not set SCD30 CO2 reference. Error code: %d", error);
            k_work_schedule(&trigger_forced_calibration_work, K_NO_WAIT);
        }
    } while (1);

    return;
}

inline void present_data_callback()
{
    SensorModelSCD30 scd30_model;

    sensor_channel_get(scd30, SENSOR_CHAN_CO2,
                       &scd30_model.co2);
    sensor_channel_get(scd30, SENSOR_CHAN_AMBIENT_TEMP,
                       &scd30_model.temperature);
    sensor_channel_get(scd30, SENSOR_CHAN_HUMIDITY,
                       &scd30_model.humidity);

    printk("CO2: %d ppm; Temperature: %d.%02d oC; Humidity: %d.%02d %% RH;\n",
           scd30_model.co2.val1,
           scd30_model.temperature.val1,
           scd30_model.temperature.val2 / 10000,
           scd30_model.humidity.val1,
           scd30_model.humidity.val2 / 10000);
}