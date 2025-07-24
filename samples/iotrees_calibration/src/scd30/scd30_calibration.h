#ifndef SCD30_CALIBRATION_H
#define SCD30_CALIBRATION_H
#include <zephyr/drivers/sensor.h>

#define SCD30_SAMPLING_RATE 10000 // 5 seconds
#define SCD30_CO2_REFERENCE 420 // Reference CO2 level for calibration in ppm
#define SCD30_TEMPERATURE_OFFSET 0.0 // Temperature offset in degrees Celsius
#define SCD30_PRESSURE 1013.25 // Pressure in hPa for calibration

typedef struct
{
	struct sensor_value co2;
	struct sensor_value temperature;
	struct sensor_value humidity;
} SensorModelSCD30;

/**
 * @brief Gets the SCD30 sensor device instance and guarantees it is ready.
 */
int init_scd30();

/**
 * @brief Enables the SCD30 sensor's low power mode.
 * 
 * This function handles the activation of the low power mode for the SCD30 sensor,
 * starting a procedure that performs: disabling the automatic self-calibration,
 * setting a sampling rate (configured in the device tree), forcing a recalibration, 
 * and finally setting the temperature offset.
 * 
 * @return 0 on success, or a negative error code on failure.
 */
int enable_scd30_low_power_mode();

#endif /* SCD30_CALIBRATION_H */