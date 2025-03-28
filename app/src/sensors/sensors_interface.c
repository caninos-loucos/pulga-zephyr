#include <zephyr/logging/log.h>
#include <sensors/sensors_interface.h>
#include <sensors/si1133/si1133_service.h>
#include <sensors/bme280/bme280_service.h>
#include <sensors/bmi160/bmi160_service.h>
#include <sensors/scd30/scd30_service.h>
#include <sensors/l86_m33/l86_m33_service.h>

LOG_MODULE_REGISTER(sensors_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// Stack of reading sensors thread
static K_THREAD_STACK_DEFINE(sensors_thread_stack_area, SENSORS_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread sensors_thread_data;
static k_tid_t sensors_thread_id;
// Time between measurements
// #TODO: Make it configurable from module that receives commands
// #TODO: Look for macro that defines variable
static int current_sampling_interval = CONFIG_SAMPLING_INTERVAL;
// List of registered sensor APIs
SensorAPI *sensor_apis[MAX_SENSORS] = {0};

// Initializes all sensors
static void init_sensors();
// Starts sensoring work - reading registered sensors measurements
static void start_reading();
// Functions that calls registered sensors in separate thread
static void perform_read_sensors(void *, void *, void *);

/**
 * IMPLEMENTATIONS
 */

int register_sensors_callbacks()
{
	LOG_DBG("Registering sensors callbacks");
#ifdef CONFIG_BME280
	sensor_apis[BME280] = register_bme280_callbacks();
#endif /* CONFIG_BME280 */

#ifdef CONFIG_BMI160
	sensor_apis[BMI160] = register_bmi160_callbacks();
#endif /* CONFIG_BMI160 */

#ifdef CONFIG_SI1133
	sensor_apis[SI1133] = register_si1133_callbacks();
#endif /* CONFIG_SI1133 */

#ifdef CONFIG_SHIELD_SCD30
	sensor_apis[SCD30] = register_scd30_callbacks();
#endif /* CONFIG_SCD30 */

#ifdef CONFIG_SHIELD_PULGA_GPS
	sensor_apis[L86_M33] = register_l86_m33_callbacks();
#endif /* CONFIG_SHIELD_PULGA_GPS */

	return 0;
}

int read_sensors()
{
	init_sensors();
	start_reading();
	return 0;
}

void init_sensors()
{
	LOG_DBG("Initializing sensors");

	int error;
	// Calls initialization function for each registered API
	for (int i = 0; i < MAX_SENSORS; i++)
	{
		error = 0;
		if (sensor_apis[i] == NULL)
		{
			continue;
		}
		error = sensor_apis[i]->init_sensor();
		if (error)
		{
			sensor_apis[i] = NULL;
		}
	}
}

void start_reading()
{
	LOG_DBG("Initializing reading thread");
	int error = 0;
	// Creates thread and starts it immediately
	sensors_thread_id = k_thread_create(&sensors_thread_data, sensors_thread_stack_area,
										K_THREAD_STACK_SIZEOF(sensors_thread_stack_area),
										perform_read_sensors, NULL, NULL,
										NULL, SENSORS_THREAD_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(sensors_thread_id, "read_sensor");
	if (error)
	{
		LOG_ERR("Failed to set read buffer thread name: %d", error);
	}
}

static void perform_read_sensors(void *param0, void *param1, void *param2)
{
	LOG_INF("Reading sensors thread started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);

	while (1)
	{
		// Calls read function for each registered API
		for (int i = 0; i < MAX_SENSORS; i++)
		{
			if (sensor_apis[i] != NULL)
			{
				sensor_apis[i]->read_sensor_values();
			}
		}
		// Waits to measure again
		k_sleep(K_MSEC(current_sampling_interval));
	}
}

// Set the interval in milliseconds between samples
void set_sampling_interval(int new_interval)
{
	current_sampling_interval = new_interval;
	LOG_DBG("Sampling interval set to %dms", new_interval);
}

// Get the interval in milliseconds between samples
int get_sampling_interval()
{
	return current_sampling_interval;
}