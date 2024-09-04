#include <zephyr/logging/log.h>

#include <sensors/sensors_interface.h>
#include <data_processing/data_abstraction.h>
#include <communication/uart/uart_interface.h>

// change log level in debug.conf
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	LOG_DBG("Starting application");
	if(read_sensors()){
		LOG_ERR("Couldn't start sensors.");
	}
	if(insert_in_buffer()){
		LOG_ERR("Couldn't start buffer.");
	}
	if(send_data()){
		LOG_ERR("Couldn't start communication.");
	}
	k_sleep(K_FOREVER);
	return 0;
}