#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include "lorawan_interface.h"

/* 	Get the LoraWAN keys from file in main directory, in the following format:

    #define LORAWAN_DEV_EUI		{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01 }
    #define LORAWAN_JOIN_EUI	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01 }
    #define LORAWAN_DEV_ADDR	0xabcdef01
    #define LORAWAN_NET_KEY 	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }
    #define LORAWAN_APP_KEY 	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }
*/
#include <lorawan_keys.h>

#if (CONFIG_LORAWAN_DR == 5)
    #define LORAWAN_DR LORAWAN_DR_5
#elif (CONFIG_LORAWAN_DR == 4)
    #define LORAWAN_DR LORAWAN_DR_4
#elif (CONFIG_LORAWAN_DR == 3)
    #define LORAWAN_DR LORAWAN_DR_3
#elif (CONFIG_LORAWAN_DR == 2)
    #define LORAWAN_DR LORAWAN_DR_2
#elif (CONFIG_LORAWAN_DR == 1)
    #define LORAWAN_DR LORAWAN_DR_1
#elif (CONFIG_LORAWAN_DR == 0)
    #define LORAWAN_DR LORAWAN_DR_0
#else
    #error "Datarate between 0 and 5 must be chosen"
#endif

// This is the region for American Tower's gateways, that's totally subject to change in the near future...
#define LORAWAN_SELECTED_REGION LORAWAN_REGION_LA915

LOG_MODULE_REGISTER(lorawan_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// Implementation of ChannelAPI for lorawan channel
static ChannelAPI lorawan_api;
// Stack of lorawan communication thread
static K_THREAD_STACK_DEFINE(lorawan_thread_stack_area, LORAWAN_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread lorawan_thread_data;
static k_tid_t lorawan_thread_id;

//TODO implement buffer
//uint8_t lorawan_priv_buffer[LORAWAN_BUFFER_SIZE];

// Initializes and starts thread to send data via lorawan
static void lorawan_init_channel();
// Functions that prints data to lorawan in separate thread
static void lorawan_send_data(void *, void *, void *);

/**
 * IMPLEMENTATIONS
 */

// Downlink callback, dumps the receiving data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
static void dl_callback(uint8_t port, bool data_pending,
			int16_t rssi, int8_t snr,
			uint8_t len, const uint8_t *hex_data)
{
	LOG_DBG("Port %d, Pending %d, RSSI %ddB, SNR %ddBm", port, data_pending, rssi, snr);
	if (hex_data) {
		LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
	}
}

static void lorawan_init_channel()
{
	const struct device *lora_dev;
	struct lorawan_join_config join_cfg;

    int ret = 0;

    LOG_DBG("Initializing send via lorawan thread");

	// Initialize the security parameters from the defined values
	uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	uint8_t app_key[] = LORAWAN_APP_KEY;
	uint8_t nwk_key[] = LORAWAN_NET_KEY;
	uint8_t app_eui[] = LORAWAN_JOIN_EUI;
	uint32_t dev_addr = LORAWAN_DEV_ADDR;

	// Initialize the callback
	struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = dl_callback
	};
	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return;
	}

	// Set the chosen region, this MUST be the same as the gateway's!
	ret = lorawan_set_region(LORAWAN_SELECTED_REGION);
	if (ret < 0) {
		LOG_ERR("lorawan_set_region failed: %d", ret);
		return;
	}
	
	// Starts the LoRaWAN backend, transmission does not start just yet
	ret = lorawan_start();
	if (ret < 0) {
		LOG_ERR("lorawan_start failed: %d", ret);
		return;
	}

	// Register the callbacks before joining the network
	lorawan_register_downlink_callback(&downlink_cb);

	// Configure the authentication parameters and join the network
	join_cfg.mode           = LORAWAN_ACT_ABP;
	join_cfg.dev_eui 		= dev_eui;
	join_cfg.abp.app_skey 	= app_key;
	join_cfg.abp.nwk_skey 	= nwk_key;
	join_cfg.abp.app_eui	= app_eui;
	join_cfg.abp.dev_addr 	= dev_addr;

	LOG_DBG("Joining network over abp");
	ret = lorawan_join(&join_cfg);
	if (ret < 0) {
		LOG_ERR("lorawan_join_network failed: %d", ret);
		return ;
	}

    // Set the initial or fixed datarate according to the config
    lorawan_set_datarate(LORAWAN_DR);

    // After joining successfully, create the send thread.
    lorawan_thread_id = k_thread_create(&lorawan_thread_data, lorawan_thread_stack_area,
                                     K_THREAD_STACK_SIZEOF(lorawan_thread_stack_area),
                                     lorawan_send_data, NULL, NULL, NULL,
                                     LORAWAN_THREAD_PRIORITY, 0, K_NO_WAIT);
    ret = k_thread_name_set(lorawan_thread_id, "send_lorawan");
    if (ret)
    {
        LOG_ERR("Failed to set read buffer thread name: %d", ret);
    }

    // TODO: create a workqueue that wakes every X seconds to send all packets inside the lorawan buffer
}

static void lorawan_send_data(void *param0, void *param1, void *param2)
{
    LOG_INF("Sending via lorawan started");
    ARG_UNUSED(param0);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    uint8_t max_size;
    uint8_t unused;

    int size, ret;

    while (1)
    {
        // Waits for data to be ready
        k_sem_take(&data_ready_sem[LORAWAN], K_FOREVER);

        lorawan_get_payload_sizes(&unused, &max_size);
        uint8_t encoded_data[max_size];

        // Encoding data to minimal string
        size = encode_data(data_unit.data_words, data_unit.data_type, MINIMALIST,
                            encoded_data, sizeof(encoded_data));
        if (size >= 0)
        {
            LOG_DBG("Encoded data: \"%s\", size %d bytes", encoded_data, size);

            ret = lorawan_send(0, encoded_data, size, LORAWAN_MSG_UNCONFIRMED);

            if (ret)
                LOG_ERR("lorawan_send failed: %d.", ret);
            else 
                LOG_INF("lorawan_send successful");
        } 
        else
            LOG_ERR("Could not encode data");

        // TODO: send the MINIMAL ENCODED DATA to internal buf

        // Signals back that lorawan sending is complete
        k_sem_give(&data_processed);
    }
}

ChannelAPI *register_lorawan_callbacks()
{
    LOG_DBG("Initializing lorawan callbacks");
    lorawan_api.init_channel = lorawan_init_channel;
    return &lorawan_api;
}