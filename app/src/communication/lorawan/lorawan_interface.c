/*
Lorawan Interface Module
Authors: Ana Clara Forcelli <ana.forcelli@lsitec.org.br>

This module is composed of four main elements:

	- Internal Buffer
	- Initialization
	- Process Data Thread
	- Send Thread

The order in which everything happens is the following:

	1 - The Initialization will set the lorawan parameters such as region, datarate and security configuration.
		It will also get the internal structures ready for use, such as the thread that process data from
		data module buffer and the thread that will effectively send the data via LoRaWAN.

	2 - Immediately after being created, the Process Data Thread is started. It will check for the signal that a data
		item was read on the data module buffer. When data is available, the thread will encode it minimally,
		using few characters. The encoded data is stored on the Internal Buffer for later transmission, so the
		LoRaWAN thread doesn't delay other transmissions because of its synchronous communication. Then,
		the thread signals for the data module that the data was processed so it can continue reading other items.

	3 - The Send Data Thread is wakened up by the Process Data Thread, and will asynchronously
		execute the actual transmission via Zephyr's send_lorawan() function.

*/

#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <communication/lorawan/lorawan_interface.h>

/* 	Get the LoraWAN keys from file in main directory, in the following format:

	#define LORAWAN_DEV_EUI		{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01 }
	#define LORAWAN_APP_EUI  	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01 }
	#define LORAWAN_DEV_ADDR	0xabcdef01
	#define LORAWAN_NET_KEY 	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }
	#define LORAWAN_APP_KEY 	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }
*/
#include <communication/lorawan/lorawan_keys_example.h>

LOG_MODULE_REGISTER(lorawan_interface, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Instance of ChannelAPI for lorawan channel
static ChannelAPI lorawan_api;

// Defines the internal buffer for sending
RING_BUF_ITEM_DECLARE(lorawan_internal_buffer, LORAWAN_BUFFER_SIZE);

// Stack of the thread that takes data read from general buffer and prepares them to send
static K_THREAD_STACK_DEFINE(lorawan_thread_stack_area, LORAWAN_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread lorawan_thread_data;
static k_tid_t lorawan_thread_id;
// Stack of the thread that actually sends the data via LoRaWAN
static K_THREAD_STACK_DEFINE(lorawan_send_thread_stack_area, LORAWAN_SEND_THREAD_STACK_SIZE);
static struct k_thread lorawan_send_thread_data;
static k_tid_t lorawan_send_thread_id;
// Semaphore to wake send data thread
static struct k_sem data_available;

// Initializes and starts thread to send data via LoRaWAN
static void lorawan_init_channel();
// Downlink callback, dumps the received data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
void downlink_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
					   uint8_t length, const uint8_t *hex_data);
// Set security configuration parameters for joining network
void lorawan_config_activation(struct lorawan_join_config *join_config);
// Functions that receives data from application buffer and
// inserts it in LoRaWAN internal buffer
static void lorawan_process_data(void *, void *, void *);
// This is the function that actually sends the data
static void lorawan_send_data(void *, void *, void *);
// Initialize the callback
static struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = downlink_callback};;

/**
 * Definitions
 */

static void lorawan_init_channel()
{
	LOG_DBG("Initializing LoRaWAN channel");

	const struct device *lora_device;
	int error = 0;

	lora_device = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_device))
	{
		LOG_ERR("%s: device not ready.", lora_device->name);
		goto return_clause;
	}

	// Set the chosen region, this MUST be the same as the gateway's!
	error = lorawan_set_region(LORAWAN_SELECTED_REGION);
	if (error)
	{
		LOG_ERR("lorawan_set_region failed: %d", error);
		goto return_clause;
	}

	// Register the callbacks before joining the network
	lorawan_register_downlink_callback(&downlink_cb);

	// Starts the LoRaWAN backend, transmission does not start just yet
	error = lorawan_start();
	if (error)
	{
		LOG_ERR("lorawan_start failed: %d", error);
		goto return_clause;
	}

	// Configuration structure to join network
	struct lorawan_join_config join_config;
	lorawan_config_activation(&join_config);
	error = lorawan_join(&join_config);
	if (error)
	{
		LOG_ERR("lorawan_join_network failed: %d", error);
		goto return_clause;
	}

	// Set the initial or fixed datarate according to the config
	error = lorawan_set_datarate(LORAWAN_DR);
	if (error)
	{
		LOG_ERR("lorawan_set_datarate failed: %d", error);
		goto return_clause;
	}

	LOG_DBG("Initializing LoRaWAN processing data thread");
	// After joining successfully, create the send thread.
	lorawan_thread_id = k_thread_create(&lorawan_thread_data, lorawan_thread_stack_area,
										K_THREAD_STACK_SIZEOF(lorawan_thread_stack_area),
										lorawan_process_data, NULL, NULL, NULL,
										LORAWAN_THREAD_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(lorawan_thread_id, "lorawan_process_data");
	if (error)
	{
		LOG_ERR("Failed to set LoRaWAN processing data thread name: %d", error);
		goto return_clause;
	}

	LOG_DBG("Initializing send via LoRaWAN thread");
	// After joining successfully, create the send thread.
	lorawan_send_thread_id = k_thread_create(&lorawan_send_thread_data, lorawan_send_thread_stack_area,
											 K_THREAD_STACK_SIZEOF(lorawan_send_thread_stack_area),
											 lorawan_send_data, NULL, NULL, NULL,
											 LORAWAN_SEND_THREAD_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(lorawan_thread_id, "lorawan_send_data");
	if (error)
	{
		LOG_ERR("Failed to set send via LoRaWAN thread name: %d", error);
		goto return_clause;
	}

	error = k_sem_init(&data_available, 0, 1);
	if (error)
	{
		LOG_ERR("Failed to initialize data available semaphore: %d", error);
		goto return_clause;
	}

return_clause:
	return;
}

// Encoding and buffering Data thread
void lorawan_process_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Processing LoRaWAN data started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	uint8_t max_payload_size;
	uint8_t unused_arg;

	int size, error;

	while (1)
	{
		// Waits for data to be ready
		k_sem_take(&data_ready_sem[LORAWAN], K_FOREVER);

		lorawan_get_payload_sizes(&unused_arg, &max_payload_size);
		uint8_t encoded_data[max_payload_size];

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

		k_sem_give(&data_available);

		// Signals back that lorawan processing is complete
		k_sem_give(&data_processed);
	}
}

// This is the function that actually sends the data
void lorawan_send_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Sending via lorawan started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	int error;
	uint16_t type;
	uint8_t value, size;
	uint32_t encoded_data[64];

	while (1)
	{
		// Waits for data to be available in internal buffer
		k_sem_take(&data_available, K_FOREVER);
		// After waking up, transmits until buffer is empty
		while (ring_buf_is_empty(&lorawan_internal_buffer) == false)
		{
			error = 0;
			size = 64;
			// Get the last message from the internal buffer
			error = ring_buf_item_get(&lorawan_internal_buffer, &type, &value, encoded_data, &size);
			if (!error)
			{
				LOG_DBG("Sending encoded data: \"%s\", with %d bytes", (char *)encoded_data, size * 4);

				// Send using Zephyr's subsystem and check if the transmission was successful
				error = lorawan_send(1, (uint8_t *)encoded_data, size * 4, LORAWAN_MSG_UNCONFIRMED);
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
}

// Downlink callback, dumps the received data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
void downlink_callback(uint8_t port, bool data_pending,
					   int16_t rssi, int8_t snr,
					   uint8_t length, const uint8_t *hex_data)
{
	LOG_DBG("Port %d, Pending %d, RSSI %ddBm, SNR %ddB", port, data_pending, rssi, snr);
	if (hex_data)
	{
		LOG_HEXDUMP_INF(hex_data, length, "Payload: ");
	}
}

// Set security configuration parameters for joining network
void lorawan_config_activation(struct lorawan_join_config *join_config)
{
	// Initialize the security parameters from the defined values
	static uint8_t device_id[] = LORAWAN_DEV_EUI;
	static uint8_t application_key[] = LORAWAN_APP_KEY;
	static uint8_t network_key[] = LORAWAN_NET_KEY;
	static uint8_t application_id[] = LORAWAN_APP_EUI;
	static uint32_t device_address = LORAWAN_DEV_ADDR;

	join_config->dev_eui = device_id;
#if defined(CONFIG_LORAWAN_OTAA)
	join_config->mode = LORAWAN_ACT_OTAA;
	join_config->otaa.join_eui = application_id;
	join_config->otaa.app_key = application_key;
	join_config->otaa.nwk_key = network_key;
	join_config->otaa.dev_nonce = 0u;
	LOG_DBG("Joining network over OTAA");
#elif defined(CONFIG_LORAWAN_ABP)
	join_config->mode = LORAWAN_ACT_ABP;
	join_config->abp.app_eui = application_id;
	join_config->abp.app_skey = application_key;
	join_config->abp.nwk_skey = network_key;
	join_config->abp.dev_addr = device_address;
	LOG_DBG("Joining network over ABP");
#endif
}

// Register channels to the Communication Module
ChannelAPI *register_lorawan_callbacks()
{
	LOG_DBG("Initializing lorawan callbacks");
	lorawan_api.init_channel = lorawan_init_channel;
	return &lorawan_api;
}