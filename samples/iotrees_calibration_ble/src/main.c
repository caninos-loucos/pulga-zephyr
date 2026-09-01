#include <stdio.h>
#include <zephyr/logging/log.h>
#include <scd30/scd30_setup.h>
#include <ble/ble_setup.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>

LOG_MODULE_REGISTER(main, CONFIG_CALIBRATION_LOG_LEVEL);

int main(void)
{
    int error;
	
    init_scd30();
    
    error = enable_scd30_low_power_mode();
	if (error)
	{
		LOG_DBG("Error enabling SCD30 low power mode: %d\n", error);
		return error;
	}

	error = bt_enable(NULL);

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
	return 0;
}
