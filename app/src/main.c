#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <drivers/si1133.h>
#include "communication/bluetooth/comm_interface.c"

// change log level in debug.conf
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

typedef struct
{
	struct
	{
		struct sensor_value temperature;
		struct sensor_value pressure;
		struct sensor_value humidity;
		int error;
	} bme280;
	struct
	{
		struct sensor_value light;
		struct sensor_value infrared;
		struct sensor_value uv;
		struct sensor_value uv_index;
		int error;
	} si1133;
} all_sensors_data;

typedef struct
{
	const struct device *bme280;
	const struct device *si1133;
	all_sensors_data all_sensors_data;
	struct k_mutex lock;
	struct k_sem data_ready;
} all_sensors_type;

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define SENSORS_THREAD_STACK_SIZE 1024
#define SENSORS_THREAD_PRIORITY 5 /* preemptible */

static K_THREAD_STACK_DEFINE(sensors_thread_stack_area, SENSORS_THREAD_STACK_SIZE);
static void sensors_thread_function(void *, void *, void *);

int main(void)
{
	/* Sensors type and data variables are not stored in the stack,
	Static variables preserve their value even outside the scope */
	static all_sensors_type all_sensors = {
		.bme280 = DEVICE_DT_GET_ANY(bosch_bme280),
		.si1133 = DEVICE_DT_GET_ANY(silabs_si1133),
	};
	static all_sensors_data data;

	if (!all_sensors.bme280)
	{
		LOG_ERR("bme280 not declared at device tree");
		return 0;
	}
	if (!all_sensors.si1133)
	{
		LOG_ERR("si1133 not declared at device tree");
		return 0;
	}
	if (!device_is_ready(all_sensors.bme280))
	{
		LOG_ERR("device \"%s\" is not ready", all_sensors.bme280->name);
		return 0;
	}
	if (!device_is_ready(all_sensors.si1133))
	{
		LOG_ERR("device \"%s\" is not ready", all_sensors.si1133->name);
		return 0;
	}

	k_mutex_init(&all_sensors.lock);
	k_sem_init(&all_sensors.data_ready, 0, 1);

	//Thread control block - metadata
	struct k_thread sensors_thread_data;
	k_tid_t sensors_thread_id;

	/* Create thread and start it immediately. */
	sensors_thread_id = k_thread_create(&sensors_thread_data, sensors_thread_stack_area,
							 K_THREAD_STACK_SIZEOF(sensors_thread_stack_area),
							 sensors_thread_function, (void *)&all_sensors, NULL, NULL,
							 SENSORS_THREAD_PRIORITY, 0, K_NO_WAIT);

	comm_channel_enum comm_channels[3] = {-1};
	comm_channels[0] = BLE;
	distribute_data("testing", comm_channels, 1);
	while (1)
	{
		LOG_DBG("waiting data");
		//Waits indefinitely until sensor measurements are ready
		k_sem_take(&all_sensors.data_ready, K_FOREVER);
		LOG_DBG("data ready");

		/* fetch available data */
		//Start of critical region
		k_mutex_lock(&all_sensors.lock, K_FOREVER);
		data = all_sensors.all_sensors_data; /* copy from shared data */
		k_mutex_unlock(&all_sensors.lock);
		//End of critical region

		/* print data */
		if (data.bme280.error < 0)
		{
			LOG_ERR("fetch sample from \"%s\" failed: %d",
					all_sensors.bme280->name, data.bme280.error);
		}
		else
		{
			printk("Temperature: %d.%02d oC; Pressure: %d.%02d kPa; "
				   "Humidity: %d.%02d %%RH;\n",
				   data.bme280.temperature.val1, data.bme280.temperature.val2 / 10000,
				   data.bme280.pressure.val1, data.bme280.pressure.val2 / 10000,
				   data.bme280.humidity.val1,
				   data.bme280.humidity.val2 / 10000);
		}
		if (data.si1133.error < 0)
		{
			LOG_ERR("fetch sample from \"%s\" failed: %d",
					all_sensors.si1133->name, data.si1133.error);
		}
		else
		{
			printk("Light: %d lux; Infrared: %d lux; UV: %d; UVIndex: %d.%02d;\n",
				   data.si1133.light.val1, data.si1133.infrared.val1,
				   data.si1133.uv.val1, data.si1133.uv_index.val1,
				   data.si1133.uv_index.val2 / 10000);
		}
	}
	return 0;
}

static void sensors_thread_function(void *param0, void *param1, void *param2)
{
	all_sensors_type *all_sensors = (all_sensors_type *)(param0);
	all_sensors_data data;
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);

	while (1)
	{
		data.bme280.error = sensor_sample_fetch(all_sensors->bme280);
		data.si1133.error = sensor_sample_fetch(all_sensors->si1133);

		if (!data.bme280.error)
		{
			sensor_channel_get(all_sensors->bme280, SENSOR_CHAN_AMBIENT_TEMP,
							   &data.bme280.temperature);
			sensor_channel_get(all_sensors->bme280, SENSOR_CHAN_PRESS,
							   &data.bme280.pressure);
			sensor_channel_get(all_sensors->bme280, SENSOR_CHAN_HUMIDITY,
							   &data.bme280.humidity);
		}

		if (!data.si1133.error)
		{
			sensor_channel_get(all_sensors->si1133, SENSOR_CHAN_LIGHT,
							   &data.si1133.light);
			sensor_channel_get(all_sensors->si1133, SENSOR_CHAN_IR,
							   &data.si1133.infrared);
			sensor_channel_get(all_sensors->si1133, SENSOR_CHAN_UV,
							   &data.si1133.uv);
			sensor_channel_get(all_sensors->si1133, SENSOR_CHAN_UVI,
							   &data.si1133.uv_index);
		}
		//Start of critical region
		k_mutex_lock(&all_sensors->lock, K_FOREVER);
		all_sensors->all_sensors_data = data; /* copy to shared data */
		k_mutex_unlock(&all_sensors->lock);
		//End of critical region

		//Notify measurements are ready with semaphore
		k_sem_give(&all_sensors->data_ready);
		k_sleep(K_MSEC(1000));
	}
}
