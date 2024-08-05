#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <drivers/si1133.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

typedef struct {
	struct {
		struct sensor_value temp;
		struct sensor_value press;
		struct sensor_value humidity;
		int error;
	} bme280;
	struct {
		struct sensor_value light;
		struct sensor_value ir;
		struct sensor_value uv;
		struct sensor_value uvi;
		int error;
	} si1133;
} mySensorData;

typedef struct {
	const struct device *bme280;
	const struct device *si1133;
	mySensorData data;
	struct k_mutex lock;
	struct k_sem data_ready;
} mySensorType;

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define MY_STACK_SIZE 1024
#define MY_PRIORITY 5 /* preemptible */

static K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);
static void my_thread_func(void *, void *, void *);

int main(void)
{
	/* mySensor and mySensorData are not stored in the stack */
	static mySensorType mySensor = { \
		.bme280 = DEVICE_DT_GET_ANY(bosch_bme280),
		.si1133 = DEVICE_DT_GET_ANY(silabs_si1133),
	};
	static mySensorData data;
	
	struct k_thread my_thread_data;
	k_tid_t my_tid;
	
	if (!mySensor.bme280) {
		LOG_ERR("bme280 not declared at device tree");
		return 0;
	}
	if (!mySensor.si1133) {
		LOG_ERR("si1133 not declared at device tree");
		return 0;
	}
	if (!device_is_ready(mySensor.bme280)) {
		LOG_ERR("device \"%s\" is not ready", mySensor.bme280->name);
		return 0;
	}
	if (!device_is_ready(mySensor.si1133)) {
		LOG_ERR("device \"%s\" is not ready", mySensor.si1133->name);
		return 0;
	}
	
	k_mutex_init(&mySensor.lock);
	k_sem_init(&mySensor.data_ready, 0, 1);
	
	/* Create our thread and start it immediately. */
	my_tid = k_thread_create(&my_thread_data, my_stack_area,
	                         K_THREAD_STACK_SIZEOF(my_stack_area),
	                         my_thread_func, (void*)&mySensor, NULL, NULL,
	                         MY_PRIORITY, 0, K_NO_WAIT);
	
	while (1) {
		LOG_DBG("waiting data");
		k_sem_take(&mySensor.data_ready, K_FOREVER);
		LOG_DBG("data ready");
		
		/* fetch available data */
		k_mutex_lock(&mySensor.lock, K_FOREVER);
		data = mySensor.data; /* copy from shared data */
		k_mutex_unlock(&mySensor.lock);
		
		/* print data */
		if (data.bme280.error < 0) {
			LOG_ERR("fetch sample from \"%s\" failed: %d",
			        mySensor.bme280->name, data.bme280.error);
		}
		else {
			printk("Temp: %d.%02d oC; Press: %d.%02d kPa; " \
			       "Humidity: %d.%02d %%RH;\n", 
			       data.bme280.temp.val1, data.bme280.temp.val2 / 10000,
			       data.bme280.press.val1, data.bme280.press.val2 / 10000,
			       data.bme280.humidity.val1, 
			       data.bme280.humidity.val2 / 10000);
		}
		if (data.si1133.error < 0) {
			LOG_ERR("fetch sample from \"%s\" failed: %d",
			        mySensor.si1133->name, data.si1133.error);
		}
		else {
			printk("Light: %d; IR: %d; UV: %d; UVI: %d.%02d;\n",
			       data.si1133.light.val1, data.si1133.ir.val1,
			       data.si1133.uv.val1, data.si1133.uvi.val1,
			       data.si1133.uvi.val2 / 10000);
		}
	}
	return 0;
}

static void my_thread_func(void *d0, void *d1, void *d2)
{
	mySensorType *mySensor = (mySensorType *)(d0);
	mySensorData data;
	ARG_UNUSED(d1);
	ARG_UNUSED(d2);
	
	while(1) {
		data.bme280.error = sensor_sample_fetch(mySensor->bme280);
		data.si1133.error = sensor_sample_fetch(mySensor->si1133);
		
		if (!data.bme280.error) {
			sensor_channel_get(mySensor->bme280, SENSOR_CHAN_AMBIENT_TEMP,
			                   &data.bme280.temp);
			sensor_channel_get(mySensor->bme280, SENSOR_CHAN_PRESS,
			                   &data.bme280.press);
			sensor_channel_get(mySensor->bme280, SENSOR_CHAN_HUMIDITY,
			                   &data.bme280.humidity);
		}
		
		if (!data.si1133.error) {
			sensor_channel_get(mySensor->si1133, SENSOR_CHAN_LIGHT,
				               &data.si1133.light);
			sensor_channel_get(mySensor->si1133, SENSOR_CHAN_IR,
				               &data.si1133.ir);
			sensor_channel_get(mySensor->si1133, SENSOR_CHAN_UV,
				               &data.si1133.uv);
			sensor_channel_get(mySensor->si1133, SENSOR_CHAN_UVI,
				               &data.si1133.uvi);
		}
		k_mutex_lock(&mySensor->lock, K_FOREVER);
		mySensor->data = data; /* copy to shared data */
		k_mutex_unlock(&mySensor->lock);
		k_sem_give(&mySensor->data_ready);
		k_sleep(K_MSEC(1000));
	}
}

