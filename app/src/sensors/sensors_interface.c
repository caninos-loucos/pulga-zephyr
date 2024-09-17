#include <zephyr/logging/log.h>
#include <sensors/sensors_interface.h>
#include <sensors/si1133/si1133_service.h>
#include <sensors/bme280/bme280_service.h>

LOG_MODULE_REGISTER(sensors_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// TODO: Minimize global variables use by using callbacks
static K_THREAD_STACK_DEFINE(sensors_thread_stack_area, SENSORS_THREAD_STACK_SIZE);
//Thread control block - metadata
static struct k_thread sensors_thread_data;
static k_tid_t sensors_thread_id;

// Time between measurements
// TODO: Make it configurable from module that receives commands
// TODO: Look for macro that defines variable
static int current_sampling_interval = CONFIG_SAMPLING_INTERVAL;

SensorAPI* sensors_apis[MAX_SENSORS] = {0};

// Initializes all sensors and synchronization structures
static int init_sensors();
// Creates and starts thread to perform sensor measurements
static int start_reading();
// Functions that reads sensors in separate thread
static void perform_read_sensors(void *, void *, void *);

/**
 * IMPLEMENTATIONS
 */

int register_sensors_callbacks(){
	LOG_DBG("Registering sensors callbacks");
	#if defined(CONFIG_BME280)
	sensors_apis[BME280] = register_bme280_callbacks();
	#endif /* CONFIG_BME280 */

	// #if defined(CONFIG_BMI160)
	// sensors_apis[BMI160] = register_bmi160_callbacks();
	// #endif /* CONFIG_BMI160 */

	#if defined(CONFIG_SI1133)
	sensors_apis[SI1133] = register_si1133_callbacks();
	#endif /* CONFIG_SI1133 */

	// #if defined(CONFIG_SCD30)
	// sensors_apis[SCD30] = register_scd30_callbacks();
	// #endif /* CONFIG_SCD30 */

	return 0;
}

int read_sensors(){
    if(init_sensors()){
		LOG_ERR("Error initializing sensors");
		return -1;
	}
	if(start_reading()){
		LOG_ERR("Error starting sensor measurements");
		return -2;
	}
	return 0;
}

int init_sensors(){
	// Calls initialization function for each registered API
	LOG_DBG("Initializing sensors");
	for(int i = 0; i < MAX_SENSORS; i++){
		if(sensors_apis[i] != NULL){
			sensors_apis[i]->init_sensor();
		}
	}
	return 0;
}

int start_reading(){
	LOG_DBG("Initializing reading thread");
	/* Create thread and start it immediately. */
	sensors_thread_id = k_thread_create(&sensors_thread_data, sensors_thread_stack_area,
							 K_THREAD_STACK_SIZEOF(sensors_thread_stack_area),
							 perform_read_sensors, NULL, NULL, 
							 NULL, SENSORS_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(sensors_thread_id, "read_sensor");

    return 0;
}

static void perform_read_sensors(void *param0, void *param1, void *param2){
	LOG_INF("Reading sensors thread started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);

	while (1)
	{
		// Calls read function for each registered API
		for(int i = 0; i < MAX_SENSORS; i++){
			if(sensors_apis[i] != NULL){
				sensors_apis[i]->read_sensor_values();
			}
		}
		// Waits to measure again
		k_sleep(K_MSEC(current_sampling_interval));
	}
}

// Dynamically sets current sampling interval
void set_sampling_interval(int new_interval){
	current_sampling_interval = new_interval;
}

// Gets current sampling interval
int get_sampling_interval(){
	return current_sampling_interval;
}