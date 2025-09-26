#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <communication/ble/ble_interface.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>


#define DEVICE_NAME "Test beacon"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)


static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};


LOG_MODULE_REGISTER(ble_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// Implementation of ChannelAPI for BLE channel
static ChannelAPI ble_api;
// Stack of BLE communication thread
static K_THREAD_STACK_DEFINE(ble_thread_stack_area, BLE_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread ble_thread_data;
static k_tid_t ble_thread_id;

// Initializes and starts thread to send data via BLE
static void ble_init_channel();
// Functions that prints data to BLE in separate thread
static void ble_send_data(void *, void *, void *);


static uint8_t ble_data[25] = { 0xff, 0xff, 0x00 , 0x01, 0x02, 0x03, 0x04, 0x05,
                                  0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
                                  0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
                                  0x12, 0x13, 0x14, 0x15, 0x16};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, ble_data, sizeof(ble_data)),
};


/**
 * IMPLEMENTATIONS
 */

static void ble_init_channel()
{
    LOG_DBG("Initializing send via BLE thread");
    
    int err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return 0;
	}
    
    int ret = 0;
    // Create thread and starts it immediately
    ble_thread_id = k_thread_create(&ble_thread_data, ble_thread_stack_area,
                                     K_THREAD_STACK_SIZEOF(ble_thread_stack_area),
                                     ble_send_data, NULL, NULL, NULL,
                                     BLE_THREAD_PRIORITY, 0, K_NO_WAIT);
    ret = k_thread_name_set(ble_thread_id, "send_ble");
    if (ret)
    {
        LOG_ERR("Failed to set read buffer thread name: %d", ret);
    }
}

static void ble_send_data(void *param0, void *param1, void *param2)
{
    LOG_DBG("Sending via BLE started");
    ARG_UNUSED(param0);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    // Max fprintf character output is 4096
    uint8_t encoded_data[25];
    int size;

    while (1)
    {
        // Waits data to be ready
        k_sem_take(&data_ready_sem[BLE], K_FOREVER);

        // Encoding data to verbose string
        size = encode_data(data_unit.data_words, data_unit.data_type, MINIMALIST,
                           encoded_data, sizeof(encoded_data));
        if (size >= 0)
        {

            memcpy(ble_data, encoded_data, sizeof(encoded_data));
            LOG_HEXDUMP_INF(ble_data, sizeof(ble_data), "My buffer");
            
            int err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
                     sd, ARRAY_SIZE(sd));
            if (err) {
                LOG_ERR("Advertising failed to start (err %d)\n", err);
                return;
            }
            k_msleep(1000);

            err = bt_le_adv_stop();
		    if (err) {
			    LOG_ERR("Advertising failed to stop (err %d)\n", err);
			return 0;
		    }
        }
        else
            LOG_ERR("Could not encode data");

        // Signals back that BLE sending is complete
        k_sem_give(&data_processed);
    }
}

ChannelAPI *register_ble_callbacks()
{
    LOG_DBG("Initializing BLE callbacks");
    ble_api.init_channel = ble_init_channel;
    return &ble_api;
}