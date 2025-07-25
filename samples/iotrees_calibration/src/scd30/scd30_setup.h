#ifndef SCD30_SETUP_H
#define SCD30_SETUP_H
#include <zephyr/drivers/sensor.h>

/**
 * The datasheets of the SCD30 sensor specify the following response times and
 * current consumption for different sampling rates:
 * 
 *  SAMPLING RATE	|	RESPONSE TIME	|	CURRENT CONSUMPTION
 * 		2s					20s						19mA
 * 		15s					72s						6.5mA
 * 		30s					135s					5.6mA
 * 
 * Using those points to interpolate a polynomial in Wolfram Alpha, we get the 
 * following function in terms of sampling rate:
 * Response time (RT) = 12.2143 + 3.87857 * x + 0.00714286 * x^2
 * 
 * For the current consumption, we interpolate the (x,y) points as (1/x, y) to get:	
 * Current consumption (CC) = 4.71175 + 4.07599/x^2 + 26.5385/x
 * 
 * Multiplying those functions, we have the power consumption in mC, which, for x>0,
 * has a local minimum between 4 and 5 seconds.
 * Considering also our empirical results on current consumption, we set the
 * sampling rate to 5 seconds.
 *
 * **Empirical results (sampling rate, current consumption):
 * {{2.0, 18.4}, {5.0, 9.51}, {10.0, 6.56}, {15.0, 5.94}, {30., 4.83}, {60., 4.4}, {120., 3.8}}
*/
#define SCD30_SAMPLING_RATE 5000 // 5 seconds
#define SCD30_CO2_REFERENCE 410 // Reference CO2 level for calibration in ppm
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

#endif /* SCD30_SETUP_H */