/*
Lorawan Interface Module
Authors: Ana Clara Forcelli <ana.forcelli@lsitec.org.br>

This module is composed of four main elements:

	- Internal Buffer
	- Workqueue
	- Initialization
	- Send Thread

The order in which everything happens is the following:

	1 - The Initialization will set the lorawan parameters such as region, datarate and authentication.
		It will also get the internal structures ready for use, such as the thread functions and workqueue.

	2 - Immediately after being called, the Send Thread is started. It will check for the signal that data
		is ready to be sent on the data module buffer. When data is available, it will encode it minimally,
		using few characters. The encoded data is stored on the Internal Buffer for later transmission, then
		the thread signals for the data module that the data is ready, in order to unblock the other channels.

	3 - The Send Thread enqueues one instance of the Work Handler to the Workqueue, which will asynchronously
		execute the actual transmission via Zephyr's send_lorawan() function.

*/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/sys/ring_buffer.h>
#include <math.h>
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

// Defines the internal buffer for sending
RING_BUF_ITEM_DECLARE(lorawan_internal_buffer, LORAWAN_BUFFER_SIZE);

LOG_MODULE_REGISTER(lorawan_interface, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Instance of ChannelAPI for lorawan channel
static ChannelAPI lorawan_api;

// Stack of lorawan communication thread
static K_THREAD_STACK_DEFINE(lorawan_thread_stack_area, LORAWAN_THREAD_STACK_SIZE);
// Stack allocated to the internal workqueue
static K_THREAD_STACK_DEFINE(lorawan_workqueue_thread_stack_area, LORAWAN_WORKQUEUE_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread lorawan_thread_data;
static k_tid_t lorawan_thread_id;

// Workqueue declarations
struct k_work lorawan_send_work;
struct k_work_q lorawan_workqueue;
struct k_sem lorawan_workqueue_semaphore;

/**
 * Definitions
 */

// Downlink callback, dumps the receiving data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
void dl_callback(uint8_t port, bool data_pending,
				 int16_t rssi, int8_t snr,
				 uint8_t len, const uint8_t *hex_data)
{
	LOG_DBG("Port %d, Pending %d, RSSI %ddB, SNR %ddBm", port, data_pending, rssi, snr);
	if (hex_data)
	{
		LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
	}
}

// This is the function that actually sends the data
void lorawan_send_work_handler(struct k_work *work)
{
	int error;
	uint16_t type;
	uint8_t value, size = 64;
	uint32_t encoded_data[64];

	error = ring_buf_is_empty(&lorawan_internal_buffer);

	if (!error)
	{
		k_sem_take(&lorawan_workqueue_semaphore, K_FOREVER);

		// Get the last message from the internal buffer
		error = ring_buf_item_get(&lorawan_internal_buffer, &type, &value, encoded_data, &size);

		k_sem_give(&lorawan_workqueue_semaphore);

		if (!error)
		{
			LOG_DBG("Sending encoded data: \"%s\", with %d bytes", (char *)encoded_data, size * 4);

			// Send using Zephyr's subsystem and check if the transmission was successful
			error = lorawan_send(0, (uint8_t *)encoded_data, size * 4, LORAWAN_MSG_UNCONFIRMED);
			if (error)
				LOG_ERR("lorawan_send failed: %d.", error);
			else
				LOG_INF("lorawan_send successful");
		}
		else
		{
			LOG_ERR("read from ring buf failed: %d.", error);
		}
	}
}

// Send Data thread
void lorawan_send_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Sending via lorawan started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	uint8_t max_size;
	uint8_t unused;

	int size, error;

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
			LOG_DBG("Encoded data: \"%s\", size %d bytes", encoded_data, size);
		else
			LOG_ERR("Could not encode data");

		// put string in internal buffer, casting it to 32-bit words
		error = ring_buf_item_put(&lorawan_internal_buffer, 0, 0, (uint32_t *)encoded_data, ceil(size / 4.0));

		if (error == -EMSGSIZE)
			LOG_INF("lorawan internal buff full");

		// Signals back that lorawan sending is complete
		k_sem_give(&data_processed);

		// Signals for async transmission
		k_work_submit_to_queue(&lorawan_workqueue, &lorawan_send_work);
	}
}

static void lorawan_init_channel()
{
	const struct device *lora_dev;
	struct lorawan_join_config join_cfg;

	int error = 0;

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
		.cb = dl_callback};
	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev))
	{
		LOG_ERR("%s: device not ready.", lora_dev->name);
		goto err;
	}
	// Set the chosen region, this MUST be the same as the gateway's!
	error = lorawan_set_region(LORAWAN_SELECTED_REGION);
	if (error)
	{
		LOG_ERR("lorawan_set_region failed: %d", error);
		goto err;
	}

	// Starts the LoRaWAN backend, transmission does not start just yet
	error = lorawan_start();
	if (error)
	{
		LOG_ERR("lorawan_start failed: %d", error);
		goto err;
	}

	// Register the callbacks before joining the network
	lorawan_register_downlink_callback(&downlink_cb);

	// Configure the authentication parameters and join the network
	join_cfg.mode = LORAWAN_ACT_ABP;
	join_cfg.dev_eui = dev_eui;
	join_cfg.abp.app_skey = app_key;
	join_cfg.abp.nwk_skey = nwk_key;
	join_cfg.abp.app_eui = app_eui;
	join_cfg.abp.dev_addr = dev_addr;

	LOG_DBG("Joining network over abp");
	error = lorawan_join(&join_cfg);
	if (error)
	{
		LOG_ERR("lorawan_join_network failed: %d", error);
		goto err;
	}

	// Set the initial or fixed datarate according to the config
	error = lorawan_set_datarate(LORAWAN_DR);
	if (error)
	{
		LOG_ERR("lorawan_set_datarate failed: %d", error);
		goto err;
	}
	// After joining successfully, create the send thread.
	lorawan_thread_id = k_thread_create(&lorawan_thread_data, lorawan_thread_stack_area,
										K_THREAD_STACK_SIZEOF(lorawan_thread_stack_area),
										lorawan_send_data, NULL, NULL, NULL,
										LORAWAN_THREAD_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(lorawan_thread_id, "send_lorawan");
	if (error)
	{
		LOG_ERR("Failed to set read buffer thread name: %d", error);
	}

	// Initialize the Workqueue and handler
	k_work_queue_init(&lorawan_workqueue);
	k_work_queue_start(&lorawan_workqueue, lorawan_workqueue_thread_stack_area,
					   K_THREAD_STACK_SIZEOF(lorawan_workqueue_thread_stack_area),
					   LORAWAN_THREAD_PRIORITY, NULL);
	k_work_init(&lorawan_send_work, lorawan_send_work_handler);

	k_sem_init(&lorawan_workqueue_semaphore, 1, 1);
err:
	return;
}

// Register channels to the Communication Module
ChannelAPI *register_lorawan_callbacks()
{
	LOG_DBG("Initializing lorawan callbacks");
	lorawan_api.init_channel = lorawan_init_channel;
	return &lorawan_api;
}