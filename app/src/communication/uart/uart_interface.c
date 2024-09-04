#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <data_processing/data_abstraction.h>
// TODO: import data type from data abstraction
#include <sensors/sensors_interface.h>
#include <communication/uart/uart_interface.h>

LOG_MODULE_REGISTER(uart_interface, CONFIG_APP_LOG_LEVEL);

int current_uart_interval = CONFIG_TRANSMISSION_INTERVAL;

static K_THREAD_STACK_DEFINE(uart_thread_stack_area, UART_THREAD_STACK_SIZE);

// Functions that prints data to UART in separate thread
static void send_data_uart(void *, void *, void *);


int send_data(){
    //Thread control block - metadata
	struct k_thread uart_thread_data;
	k_tid_t uart_thread_id;

	/* Create thread and start it immediately. */
	uart_thread_id = k_thread_create(&uart_thread_data, uart_thread_stack_area,
							 K_THREAD_STACK_SIZEOF(uart_thread_stack_area),
                             send_data_uart, NULL, NULL, NULL, 
                             UART_THREAD_PRIORITY, 0, K_NO_WAIT);
    return 0;
}

static void send_data_uart(void *param0, void *param1, void *param2){
    ARG_UNUSED(param0);
    ARG_UNUSED(param1);
	ARG_UNUSED(param2);

    SensorsData sensors_data;

    while (1)
	{
        // Waits to send data
        //TODO: or buffer full
		k_sleep(K_MSEC(current_uart_interval));

		LOG_DBG("waiting buffer");
		//Waits indefinitely until buffer has data
		k_sem_take(&data_in_buffer, K_FOREVER);
		LOG_DBG("buffer ready");

		/* fetch available data */
		//Start of critical region
        // TODO: change lock to buffer lock
		k_mutex_lock(&sensors_return.lock, K_FOREVER);
		sensors_data = sensors_return.sensors_data; /* copy from shared data */
		k_mutex_unlock(&sensors_return.lock);
		//End of critical region

		/* print data */
        #if defined(CONFIG_BME280)
		if (sensors_data.bme280.error < 0)
		{
			LOG_ERR("fetch sample from \"%s\" failed: %d",
					"BME280", sensors_data.bme280.error);
		}
		else
		{
			printk("Temperature: %d.%02d oC; Pressure: %d.%02d kPa; "
				   "Humidity: %d.%02d %%RH;\n",
				   sensors_data.bme280.temperature.val1, 
                   sensors_data.bme280.temperature.val2 / 10000,
				   sensors_data.bme280.pressure.val1, 
                   sensors_data.bme280.pressure.val2 / 10000,
				   sensors_data.bme280.humidity.val1,
				   sensors_data.bme280.humidity.val2 / 10000);
		}
        #endif /* CONFIG_BME280 */

        #if defined(CONFIG_SI1133)
		if (sensors_data.si1133.error < 0)
		{
			LOG_ERR("fetch sample from \"%s\" failed: %d",
					"Si1133", sensors_data.si1133.error);
		}
		else
		{
			printk("Light: %d lux; Infrared: %d lux; UV: %d; UVIndex: %d.%02d;\n",
				   sensors_data.si1133.light.val1, 
                   sensors_data.si1133.infrared.val1,
				   sensors_data.si1133.uv.val1, 
                   sensors_data.si1133.uv_index.val1,
				   sensors_data.si1133.uv_index.val2 / 10000);
		}
        #endif /* CONFIG_SI1133 */
	}
}