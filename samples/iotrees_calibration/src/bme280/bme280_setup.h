#ifndef BME280_SETUP_H
#define BME280_SETUP_H
#include <zephyr/drivers/sensor.h>

#define BME280_SAMPLING_RATE 1000 // 1 second
#define BME280_NUM_SAMPLES_FOR_MEAN 10.0f // Number of samples to average for better accuracy

/**
 * @brief Gets the BME280 sensor device instance and guarantees it is ready.
 */
int init_bme280();

/**
 * @brief Retrieves the mean values of the BME280 sensor last measurements of temperature.
 * 
 * This function fetches the last BME280 sensor measurements, averages them over 
 * a defined number of samples (BME280_NUM_SAMPLES_FOR_MEAN), and stores the
 * results in the float variable.
 * 
 * @return 0 on success, or a negative error code on failure.
 */
int get_mean_bme280_values(float *mean_temperature);

#endif /* BME280_SETUP_H */