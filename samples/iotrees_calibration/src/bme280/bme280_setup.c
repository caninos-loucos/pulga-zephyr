#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <bme280/bme280_setup.h>

LOG_MODULE_REGISTER(bme280_setup, CONFIG_CALIBRATION_LOG_LEVEL);

/**
 * DEFINITIONS
 */

/**
 * @brief BME280 sensor device instance.
 */
static const struct device *bme280;
/**
 * @brief This function reads and presents measurements from the BME280 sensor.
 */
static inline int read_sensor_values(float *latest_measurements);

/**
 * IMPLEMENTATIONS
 */

int init_bme280()
{
    LOG_DBG("Initializing BME280");
    bme280 = DEVICE_DT_GET_ANY(bosch_bme280);

    if (!bme280)
    {
        LOG_ERR("BME280 not declared at device tree");
        return -ENODEV;
    }
    else if (!device_is_ready(bme280))
    {
        LOG_ERR("BME280 is not ready");
        return -EAGAIN;
    }

    return 0;
}

int get_mean_bme280_values(float *mean_temperature)
{
    int error = 0;
    *mean_temperature = 0.0f;

    for (int measurement_count = 0; measurement_count < BME280_NUM_SAMPLES_FOR_MEAN; measurement_count++)
    {
        float latest_measurement = 0.0f;
        // Read the latest sensor values
        do
        {
            error = read_sensor_values(&latest_measurement);
        } while (error == -EAGAIN);

        // Accumulate the values
        *mean_temperature += latest_measurement;
        k_sleep(K_MSEC(BME280_SAMPLING_RATE));
    }

    // Calculate the mean values
    *mean_temperature = *mean_temperature / BME280_NUM_SAMPLES_FOR_MEAN;
    return 0;
}

inline int read_sensor_values(float *latest_measurements)
{
    LOG_DBG("Reading BME280 sensor values");
    struct sensor_value temperature, pressure;
    int error;

    // Fetch the latest sample
    error = sensor_sample_fetch(bme280);
    if (error < 0)
    {
        LOG_ERR("Failed to fetch sample from BME280: %d", error);
        return error;
    }

    // Get the sensor values
    sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
    sensor_channel_get(bme280, SENSOR_CHAN_PRESS, &pressure);
    *latest_measurements = sensor_value_to_float(&temperature);
    float pressure_float = sensor_value_to_float(&pressure);

    LOG_DBG("BME280 readings - Temperature: %.2f oC, Pressure: %.2f kPa",
            (double)*latest_measurements,
            (double)pressure_float);
    return 0;
}