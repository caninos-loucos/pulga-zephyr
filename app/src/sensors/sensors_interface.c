#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <drivers/si1133.h>
#include <sensors/sensors_interface.h>

LOG_MODULE_REGISTER(sensors_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// TODO: Minimize global variables use by using callbacks
static K_THREAD_STACK_DEFINE(sensors_thread_stack_area, SENSORS_THREAD_STACK_SIZE);
//Thread control block - metadata
static struct k_thread sensors_thread_data;
static k_tid_t sensors_thread_id;

static SensorsType all_sensors;
SensorsReturn sensors_return;

// Time between measurements
// TODO: Make it configurable from module that receives commands
// TODO: Look for macro that defines variable
static int current_sampling_interval = CONFIG_SAMPLING_INTERVAL;

// Initializes all sensors and synchronization structures
static int init_sensors();
// Creates and starts thread to perform sensor measurements
static int start_reading();
// Functions that reads sensors in separate thread
static void perform_read_sensors(void *, void *, void *);

/**
 * IMPLEMENTATIONS
 */

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
	int error = 0;
    /* Sensors type and data variables are not stored in the stack,
	Static variables preserve their value even outside the scope */
	LOG_DBG("Initializing sensors");
	#if defined(CONFIG_BME280)
	all_sensors.bme280 = DEVICE_DT_GET_ANY(bosch_bme280);
	#endif /* CONFIG_BME280 */

	#if defined(CONFIG_SI1133)
	all_sensors.si1133 = DEVICE_DT_GET_ANY(silabs_si1133);
	#endif /* CONFIG_SI1133 */

    #if defined(CONFIG_BME280)
	// TODO: Make it so it doesnt return when the sensors dont work
	if (!all_sensors.bme280)
	{
		LOG_ERR("bme280 not declared at device tree");
		return -1;
	}
	if (!device_is_ready(all_sensors.bme280))
	{
		LOG_ERR("device \"%s\" is not ready", all_sensors.bme280->name);
		return -11;
	}
    #endif /* CONFIG_BME280 */

	// TODO: Make it so it doesnt return when the sensors dont work
    #if defined(CONFIG_SI1133)
	if (!all_sensors.si1133)
	{
		LOG_ERR("si1133 not declared at device tree");
		return -2;
	}
	if (!device_is_ready(all_sensors.si1133))
	{
		LOG_ERR("device \"%s\" is not ready", all_sensors.si1133->name);
		return -12;
	}
    #endif /* CONFIG_SI1133 */
	
	LOG_DBG("Initializing mutexes");
	error = k_mutex_init(&sensors_return.lock);
	if(error){
		LOG_ERR("Couldn't initialize sensor data lock.");
		return -100;
	}
	error = k_sem_init(&sensors_return.data_ready, 0, 1);
	if(error){
		LOG_ERR("Couldn't initialize sensor data semaphore.");
		return -101;
	}

    return 0;
}

int start_reading(){
	LOG_DBG("Initializing reading thread");
	/* Create thread and start it immediately. */
	sensors_thread_id = k_thread_create(&sensors_thread_data, sensors_thread_stack_area,
							 K_THREAD_STACK_SIZEOF(sensors_thread_stack_area),
							 perform_read_sensors, &all_sensors, &sensors_return, 
							 NULL, SENSORS_THREAD_PRIORITY, 0, K_NO_WAIT);
    return 0;
}

static void perform_read_sensors(void *param0, void *param1, void *param2){
	LOG_INF("Reading thread started");
    SensorsType *all_sensors = (SensorsType *)(param0);
    SensorsReturn *sensors_return = (SensorsReturn *)(param1);
	SensorsData sensors_data;
	ARG_UNUSED(param2);

	while (1)
	{
		#if defined(CONFIG_BME280)
		sensors_data.bme280.error = sensor_sample_fetch(all_sensors->bme280);
		if (!sensors_data.bme280.error)
		{
			sensor_channel_get(all_sensors->bme280, SENSOR_CHAN_AMBIENT_TEMP,
							   &sensors_data.bme280.temperature);
			sensor_channel_get(all_sensors->bme280, SENSOR_CHAN_PRESS,
							   &sensors_data.bme280.pressure);
			sensor_channel_get(all_sensors->bme280, SENSOR_CHAN_HUMIDITY,
							   &sensors_data.bme280.humidity);
		}
        #endif /* CONFIG_BME280 */

        #if defined(CONFIG_SI1133)
		sensors_data.si1133.error = sensor_sample_fetch(all_sensors->si1133);
		if (!sensors_data.si1133.error)
		{
			sensor_channel_get(all_sensors->si1133, SENSOR_CHAN_LIGHT,
							   &sensors_data.si1133.light);
			sensor_channel_get(all_sensors->si1133, SENSOR_CHAN_IR,
							   &sensors_data.si1133.infrared);
			sensor_channel_get(all_sensors->si1133, SENSOR_CHAN_UV,
							   &sensors_data.si1133.uv);
			sensor_channel_get(all_sensors->si1133, SENSOR_CHAN_UVI,
							   &sensors_data.si1133.uv_index);
		}
        #endif /* CONFIG_SI1133 */

		// Start of critical region
		k_mutex_lock(&sensors_return->lock, K_FOREVER);
		sensors_return->sensors_data = sensors_data; /* copy to shared data */
		k_mutex_unlock(&sensors_return->lock);
		// End of critical region

		// Notify measurements are ready with semaphore
		k_sem_give(&sensors_return->data_ready);
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