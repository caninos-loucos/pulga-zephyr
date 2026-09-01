#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/scd30.h>
#include <scd30/scd30_setup.h>
#include <ble/ble_setup.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <math.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>


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
// Reference CO2 level for calibration in ppm
uint32_t scd30_co2_reference = 410;
// Reference temperature level for offset calibration
float temperature_reference;
// time at sample time configuration, for minimal time delay
uint32_t scd30_init_time;

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
 * Sets the temperature offset for the SCD30 sensor.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static inline void set_temperature_offset(struct k_work *work);
/**
 * Work item for deferring the forced recalibration of the SCD30 sensor until
 * it warms up (2 minutes or 5 times the sampling rate).
 */
K_WORK_DELAYABLE_DEFINE(trigger_forced_calibration_work, force_calibration);
/**
 * Work item for deferring the setting of the temperature and pressure offsets
 * for the SCD30 sensor.
 */
K_WORK_DELAYABLE_DEFINE(trigger_temperature_offset_work, set_temperature_offset);

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

    scd30_init_time = k_uptime_get();

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

ssize_t write_co2_reference(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{

    // Enforce that the write as no offset (is a single transmission)
    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    // Enforce that the data payload is exactly 2 bytes
    if (len != 2) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    scd30_co2_reference = sys_get_le16(buf); // read 2 bytes (little-endian)

    LOG_DBG("CO2 reference value (BLE calibration): %d ppm", scd30_co2_reference);

    // Schedule the forced recalibration after 5 times the sampling rate
    int calibration_delay = SCD30_SAMPLING_RATE * 5;
    calibration_delay = 0;//MAX(calibration_delay, 360000)+scd30_init_time; // Ensure at least 6 minutes delay since sensor init
    k_work_schedule(&trigger_forced_calibration_work, K_MSEC(MAX(calibration_delay-k_uptime_get(),1000)));

    return len;
}

static inline void force_calibration(struct k_work *work)
{
    int error = 0;
    struct sensor_value calibration_reference;
    LOG_DBG("Forcing SCD30 calibration...");

    LOG_INF("Setting SCD30 CO2 reference to %d ppm", scd30_co2_reference);
    calibration_reference.val1 = scd30_co2_reference;
    error = sensor_attr_set(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_FORCED_RECALIBRATION,
                            &calibration_reference);

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
        if (calibration_reference.val1 == scd30_co2_reference)
        {
            LOG_INF("SCD30 CO2 reference set to %d ppm",
                    scd30_co2_reference);
            return;
        }

        LOG_INF("Setting SCD30 CO2 reference to %d ppm", scd30_co2_reference);
        calibration_reference.val1 = scd30_co2_reference;
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

ssize_t write_temp_reference(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    // Enforce that the write as no offset (is a single transmission)
    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    // Enforce that the data payload is exactly 2 bytes
    if (len != 2) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    temperature_reference = (float)sys_get_le16(buf)/100; // read 2 bytes (little-endian) (float)temp_aux/100;

    LOG_DBG("Temperature reference value (BLE calibration): %.2f oC", (double)temperature_reference);

    int calibration_delay = 0; //600000+scd30_init_time; // Ensure at least 10 minutes delay since sensor init
    k_work_schedule(&trigger_temperature_offset_work, K_MSEC(MAX(calibration_delay-k_uptime_get(),1000)));

    return len;
}

static inline void set_temperature_offset(struct k_work *work)
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
        return;
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
        return;
    }

    error = sensor_attr_get(scd30, SENSOR_CHAN_ALL, SCD30_SENSOR_ATTR_TEMPERATURE_OFFSET,
                            &current_offset);
    if (error)
    {
        LOG_ERR("Failed to get new SCD30 temperature offset: %d", error);
        return;
    }
    current_offset_value = sensor_value_to_float(&current_offset);

    if (new_offset_value > 0 && fabs(current_offset_value-new_offset_value) > 0.05)
        {
            LOG_ERR("Temperature offset register value didnt update, %.2f oC != %.2f oC",
                    (double)current_offset_value, (double)new_offset_value);
            return;
        }

    LOG_DBG("New SCD30 temperature offset set.");
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


    char buffer[100];
    sprintf(buffer, "CO2: %.2f ppm; Temperature: %.2f oC; Humidity: %.2f %% RH;",
            (double)sensor_value_to_float(&scd30_model.co2),
            (double)sensor_value_to_float(&scd30_model.temperature),
            (double)sensor_value_to_float(&scd30_model.humidity));

    LOG_DBG("%s", buffer);

    reading_indicate(buffer);

}