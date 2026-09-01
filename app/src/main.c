#include <zephyr/logging/log.h>

#include <sensors/sensors_interface.h>
#include <integration/data_abstraction/abstraction_service.h>
#include <communication/comm_interface.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>

#include <communication/ble/ble_setup.h>

// change log level in debug.conf
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

// Registers application callbacks
int register_callbacks();

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

	int error = bt_enable(NULL);

    if (error)
    {
        LOG_DBG("Bluetooth init failed (err %d)\n \r", error);
        return error;
    }

    bt_start_advertising();

    while (1)
    {
        k_sleep(K_MSEC(1000));
        bt_start_advertising(); //re-enable advertising after disconnect
    }

	k_sleep(K_FOREVER);
	return 0;
}

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