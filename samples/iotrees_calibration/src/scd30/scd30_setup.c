#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/scd30.h>
#include <scd30/scd30_setup.h>
#include <bme280/bme280_setup.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(scd30_setup, CONFIG_CALIBRATION_LOG_LEVEL);

/**
 * DEFINITIONS
 */

/**
 * SCD30 sensor device instance.
 */
static const struct device *scd30;
/**
 * SCD30 temperature records for mean calculation.
 */
static struct SCD30_Temperature_Record
{
    float temperature_mean;
    int count;
} scd30_temperature_record = {0.0f, 0};
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
 * This function is triggered to present data from the SCD30 sensor.
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
 */
static inline void force_calibration(struct k_work *work);
/**
 * Sets the temperature and pressure offsets for the SCD30 sensor.
 */
static inline void set_environment_offsets(struct k_work *work);
/**
 * Sets the temperature offset for the SCD30 sensor.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline int set_temperature_offset(float temperature_offset);
/**
 * Work item for deferring the forced recalibration of the SCD30 sensor until
 * it warms up (2 minutes or 5 times the sampling rate).
 */
K_WORK_DELAYABLE_DEFINE(trigger_forced_calibration_work, force_calibration);
/**
 * Work item for deferring the setting of the temperature and pressure offsets
 * for the SCD30 sensor.
 */
K_WORK_DELAYABLE_DEFINE(trigger_environment_offset_work, set_environment_offsets);

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

    // Initialize measurements with the default ambient pressure
    int error = scd30_start_periodic_measurement(scd30, SCD30_SAO_PAULO_AMBIENT_PRESSURE);

    // Registers desired application callback into the scd30 driver api
    scd30_register_callback(scd30, present_data_callback);

    return error;
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

    // Uncommenting the next line will delay the calibration process
    // to allow for setting up the sensor in a stable environment.
    // k_sleep(K_MINUTES(2));

    // Schedule the forced recalibration after 5 times the sampling rate
    int calibration_delay = SCD30_SAMPLING_RATE * 5;
    calibration_delay = MAX(calibration_delay, 120000); // Ensure at least 2 minutes delay
    k_work_schedule(&trigger_forced_calibration_work, K_MSEC(calibration_delay));
    // Schedule the temperature offset setting after 10 minutes
    k_work_schedule(&trigger_environment_offset_work, SCD30_TEMPERATURE_STABILIZATION_TIME);

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
            return;
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
            return;
        }
    } while (1);

    return;
}

static inline void set_environment_offsets(struct k_work *work)
{
    int error = 0;
    float temperature_reference = 0.0f;

    LOG_DBG("Setting SCD30 environment offsets...");
    error = get_mean_bme280_values(&temperature_reference);
    if (error)
    {
        LOG_ERR("Failed to get mean BME280 values: %d", error);
        k_work_schedule(&trigger_environment_offset_work, K_NO_WAIT);
        return;
    }

    // Set the temperature offset
    error = set_temperature_offset(temperature_reference);
    if (error)
    {
        LOG_ERR("Failed to set temperature offset: %d", error);
        k_work_schedule(&trigger_environment_offset_work, K_NO_WAIT);
        return;
    }

    LOG_DBG("SCD30 environment offsets set successfully");
}

static inline int set_temperature_offset(float temperature_reference)
{
    int error = 0;
    struct sensor_value current_offset, new_offset;
    float current_offset_value, new_offset_value;

    LOG_DBG("Setting SCD30 temperature offset using reference value: %.2f oC",
            (double)temperature_reference);
    // Gets the current temperature offset
    error = sensor_attr_get(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_TEMPERATURE_OFFSET,
                            &current_offset);
    if (error)
    {
        LOG_ERR("Failed to get SCD30 temperature offset: %d", error);
        return error;
    }
    current_offset_value = sensor_value_to_float(&current_offset);

    // Calculate the new offset value
    new_offset_value = scd30_temperature_record.temperature_mean - temperature_reference +
                       current_offset_value;
    LOG_DBG("Current SCD30 temperature mean: %.2f oC", (double)scd30_temperature_record.temperature_mean);
    LOG_DBG("Current offset: %.2f oC", (double)current_offset_value);
    LOG_DBG("New offset: %.2f oC", (double)new_offset_value);
    sensor_value_from_float(&new_offset, new_offset_value);

    // Send the calculated offset to the driver
    error = sensor_attr_set(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_TEMPERATURE_OFFSET,
                            &new_offset);
    if (error)
    {
        LOG_ERR("Could not set SCD30 temperature offset. Error code: %d", error);
        return error;
    }

    LOG_DBG("New SCD30 temperature offset set.");
    return 0;
}

static inline void present_data_callback()
{
    SensorModelSCD30 scd30_model;

    sensor_channel_get(scd30, SENSOR_CHAN_CO2,
                       &scd30_model.co2);
    sensor_channel_get(scd30, SENSOR_CHAN_AMBIENT_TEMP,
                       &scd30_model.temperature);
    sensor_channel_get(scd30, SENSOR_CHAN_HUMIDITY,
                       &scd30_model.humidity);

    // Update the temperature mean value
    float new_temperature = sensor_value_to_float(&scd30_model.temperature);
    scd30_temperature_record.count++;
    scd30_temperature_record.temperature_mean = (1 - SCD30_TEMPERATURE_WEIGHT) *
                                                    scd30_temperature_record.temperature_mean +
                                                SCD30_TEMPERATURE_WEIGHT * new_temperature;

    LOG_DBG("CO2: %.2f ppm; Temperature: %.2f oC; Humidity: %.2f %% RH;",
            (double)sensor_value_to_float(&scd30_model.co2),
            (double)sensor_value_to_float(&scd30_model.temperature),
            (double)sensor_value_to_float(&scd30_model.humidity));
}