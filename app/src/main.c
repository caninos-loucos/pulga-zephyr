#include <zephyr/logging/log.h>

#include <sensors/sensors_interface.h>
#include <data_processing/data_abstraction.h>
#include <communication/uart/uart_interface.h>

// change log level in debug.conf
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int register_callbacks(){
	// register sensors callbacks
	if(register_sensors_callbacks()){
		LOG_ERR("Couldn't register sensors callback.");
		return -1;
	}
	// register channels callbacks
	if(register_comm_callbacks()){
		LOG_ERR("Couldn't register channels callbacks.");
		return -2;
	}
	// register data type callbacks
	if(register_data_callbacks()){
		LOG_ERR("Couldn't register data type callbacks.");
		return -3;
	}
	return 0;
}

int main(void)
{
	LOG_DBG("Starting application");
	if(!register_callbacks()){
		if(init_communication()){
			LOG_ERR("Couldn't start communication.");
		}
		if(read_sensors()){
			LOG_ERR("Couldn't start sensors.");
		}
	}
	k_sleep(K_FOREVER);
	return 0;
}