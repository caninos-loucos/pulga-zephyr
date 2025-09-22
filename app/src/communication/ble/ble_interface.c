#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <communication/ble/ble_interface.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>


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


static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
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
    uint8_t encoded_data[1024];
    int size;

    while (1)
    {
        // Waits data to be ready
        k_sem_take(&data_ready_sem[BLE], K_FOREVER);

        // Encoding data to verbose string
        size = encode_data(data_unit.data_words, data_unit.data_type, VERBOSE,
                           encoded_data, sizeof(encoded_data));
        if (size >= 0)
        {
            int err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
                    NULL, 0);
            if (err) {
                LOG_ERR("Advertising failed to start (err %d)\n", err);
                return;
            }
            printk("%s\n", encoded_data);
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