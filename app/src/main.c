#include <zephyr/logging/log.h>

#include <sensors/sensors_interface.h>
#include <data_processing/data_abstraction.h>
#include <communication/uart/uart_interface.h>

// change log level in debug.conf
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	LOG_INF("Starting application");
	if(read_sensors(CONFIG_SAMPLING_INTERVAL)){
		LOG_ERR("Couldn't start sensors.");
	}
	if(insert_in_buffer()){
		LOG_ERR("Couldn't start buffer.");
	}
	if(send_data(CONFIG_TRANSMISSION_INTERVAL)){
		LOG_ERR("Couldn't start communication.");
	}
	return 0;
}