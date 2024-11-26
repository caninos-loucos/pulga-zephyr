/*
Lorawan Interface Module
Authors: Ana Clara Forcelli <ana.forcelli@lsitec.org.br>

This module is composed of four main elements:

	- Internal Buffer
	- Workqueue
	- Initialization
	- Send Thread

The order in which everything happens is the following:

	1 - The Initialization will set the lorawan parameters such as region, datarate and security configuration.
		It will also get the internal structures ready for use, such as the thread that process data from
		data module buffer and the workqueue that will effectively send the data via LoRaWAN.

	2 - Immediately after being created, the Send Thread is started. It will check for the signal that a data
		item was read on the data module buffer. When data is available, the thread will encode it minimally,
		using few characters. The encoded data is stored on the Internal Buffer for later transmission, so the
		LoRaWAN thread doesn't delay other transmissions because of its synchronous communication. Then,
		the thread signals for the data module that the data was processed so it can continue reading other items.

	3 - The Send Thread enqueues one instance of the Work Handler to the Workqueue, which will asynchronously
		execute the actual transmission via Zephyr's send_lorawan() function.

*/

#ifdef CONFIG_LORAWAN_JOIN_COMPRESS
#include "lz4frame.h"
#include "lz4hc.h"
#include "lz4.h"
#endif

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

struct k_mutex buf_read_mutex;

#ifdef CONFIG_LORAWAN_JOIN_COMPRESS
double compression_factor;
#endif

// Initializes and starts thread to send data via LoRaWAN
static void lorawan_init_channel();
// Downlink callback, dumps the received data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
void downlink_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr,
					   uint8_t length, const uint8_t *hex_data);
// Callback to be used whenever datarate changes. When CONFIG_LORAWAN_JOIN_COMPRESS is set, adjusts the expected compression level.
static void dr_changed_callback(enum lorawan_datarate dr);
// Set security configuration parameters for joining network
void lorawan_config_activation(struct lorawan_join_config *join_config);
// Functions that receives data from application buffer and
// inserts it in LoRaWAN internal buffer
static void lorawan_send_data(void *, void *, void *);
// This is the function that actually sends the data
void lorawan_send_work_handler(struct k_work *work);

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

	// Initialize the callback
	struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = downlink_callback};

	// Register the callbacks before joining the network
	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(dr_changed_callback);

	// Starts the LoRaWAN backend, transmission does not start just yet
	error = lorawan_start();
	if (error)
	{
		LOG_ERR("lorawan_start failed: %d", error);
		goto return_clause;
	}

	// Set the initial or fixed datarate according to the config
	error = lorawan_set_datarate(LORAWAN_DR);
	if (error)
	{
		LOG_ERR("lorawan_set_datarate failed: %d", error);
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

	LOG_DBG("Initializing send via LoRaWAN thread");
	// After joining successfully, create the send thread.
	lorawan_thread_id = k_thread_create(&lorawan_thread_data, lorawan_thread_stack_area,
										K_THREAD_STACK_SIZEOF(lorawan_thread_stack_area),
										lorawan_send_data, NULL, NULL, NULL,
										LORAWAN_THREAD_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(lorawan_thread_id, "send_lorawan");
	if (error)
	{
		LOG_ERR("Failed to set send via LoRaWAN thread name: %d", error);
	}

	// Initialize the Workqueue and handler
	k_mutex_init(&buf_read_mutex);
	k_work_queue_init(&lorawan_workqueue);
	k_work_queue_start(&lorawan_workqueue, lorawan_workqueue_thread_stack_area,
					   K_THREAD_STACK_SIZEOF(lorawan_workqueue_thread_stack_area),
					   LORAWAN_THREAD_PRIORITY, NULL);
	k_work_init(&lorawan_send_work, lorawan_send_work_handler);

return_clause:
	return;
}

// Send Data thread
void lorawan_send_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Sending via lorawan started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	uint8_t max_payload_size;
	uint8_t unused_arg;

	int size, error;
	int pending_items = 0;
	int ring_buf_data_size;

	uint8_t encoded_data[256];

	while (1)
	{
		// Waits for data to be ready
		k_sem_take(&data_ready_sem[LORAWAN], K_FOREVER);

		lorawan_get_payload_sizes(&unused_arg, &max_payload_size);

		memset(encoded_data, 0, 256);

		// Encoding data to raw bytes
		size = encode_data(data_unit.data_words, data_unit.data_type, RAW_BYTES,
						   encoded_data, sizeof(encoded_data));

		if (size < 0)
		{
			LOG_ERR("Could not encode data");
			k_sem_give(&data_processed);
			continue;
		}

#ifdef CONFIG_LORAWAN_JOIN_COMPRESS
		else if (size * compression_factor > max_payload_size)
#else
		else if (size > max_payload_size)
#endif
		{
			LOG_ERR("Resulting size too big for lorawan packet");
			k_sem_give(&data_processed);
			continue;
		}

		// put bytes in internal buffer, casting it to 32-bit words
		error = ring_buf_item_put(&lorawan_internal_buffer, data_unit.data_type, 0, (uint32_t *)encoded_data, (size + 3) / 4);

		if (error == -EMSGSIZE)
			LOG_INF("lorawan internal buff full");
		else
			pending_items++;

#ifdef CONFIG_LORAWAN_JOIN_COMPRESS

		k_mutex_lock(&buf_read_mutex, K_FOREVER);
		// calculate the amount of data bytes inside the data buffer and subtracts 16 bits (2 bytes) for each item.
		// because datatype and value total 24 bits, but we want one byte for later identification and an EOL byte.
		ring_buf_data_size = ring_buf_size_get(&lorawan_internal_buffer) - 2 * pending_items - 1;

		k_mutex_unlock(&buf_read_mutex);

		// expected compression factor is 1.5 in a conservative estimate, since lz4 can do 2.
		if (ring_buf_data_size <= max_payload_size * compression_factor)
		{
			LOG_DBG("ITEM ADDED");
		}
		else
		{
			pending_items = 0;
			LOG_DBG("SUBMIT QUEUE");
			k_work_submit_to_queue(&lorawan_workqueue, &lorawan_send_work);
		}

#else
		// Signals for async transmission
		k_work_submit_to_queue(&lorawan_workqueue, &lorawan_send_work);

#endif

		// Signals back that lorawan sending is complete
		k_sem_give(&data_processed);
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

void dr_changed_callback(enum lorawan_datarate new_dr)
{
	LOG_INF("Datarate changed to DR_%d", (int)new_dr);
#ifdef CONFIG_LORAWAN_JOIN_COMPRESS
	// Exponential fitting based on empirical data
	switch (new_dr)
	{
	case LORAWAN_DR_0:
	case LORAWAN_DR_1:
	case LORAWAN_DR_2:
	case LORAWAN_DR_3:
		compression_factor = 1.3;
		break;
	case LORAWAN_DR_4:
	case LORAWAN_DR_5:
		compression_factor = 2;
		break;
	default:
		compression_factor = 1;
	}
	LOG_INF("Compression factor changed to %d", (int)(compression_factor * 10));
#endif
}

// This is the function that actually sends the data
void lorawan_send_work_handler(struct k_work *work)
{
	int error;

	uint16_t type;
	uint8_t value;

	uint8_t max_payload_size;
	uint8_t unused_arg;

	uint32_t encoded_data[MAX_32_WORDS];
	uint8_t encoded_data_size;

#ifdef CONFIG_LORAWAN_JOIN_COMPRESS

	uint8_t joined_data[512];
	uint8_t compressed_data[256];
	uint8_t separator[2] = {0xca, 0xfe};
	int index = 0;

	memset(joined_data, 0, sizeof(joined_data));

	lorawan_get_payload_sizes(&unused_arg, &max_payload_size);

	// Joining the data until it reaches the payload size times the expected compression factor
	k_mutex_lock(&buf_read_mutex, K_FOREVER);
	while (index < max_payload_size * compression_factor)
	{
		encoded_data_size = sizeof(encoded_data) / sizeof(uint32_t);

		error = ring_buf_item_get(&lorawan_internal_buffer, &type, &value, encoded_data, &encoded_data_size);

		// If the buffer is empty, exit the loop
		if (error == -EAGAIN)
			break;
		else if (error)
			goto buf_err;

		// Turn the encoded size to bytes
		encoded_data_size *= 4;

		// Check if the joined_data buffer can take this data item, then if it can't, break the loop and discard this item
		if (index + encoded_data_size + 1 > max_payload_size * compression_factor)
		{
			LOG_WRN("Item does not fit in joined data buffer, Discarding.");
			k_mutex_unlock(&buf_read_mutex);
			break;
		}

		// Data type is reduced to a byte, big data types aint happening here
		bytecpy(joined_data + index, &type, 1);
		index++;

		// Copy the data item to the joined_data buffer
		bytecpy(joined_data + index, encoded_data, encoded_data_size);
		index += encoded_data_size;

		// Add end of packet byte
		bytecpy(joined_data + index, separator, sizeof(separator));
		index += sizeof(separator);

		// Anticipate whether the joined_data buffer can take another measurement with
		// the same size as the last one: if it can't, break the loop and send it
		if (index + encoded_data_size + 1 > max_payload_size * compression_factor)
		{
			k_mutex_unlock(&buf_read_mutex);
			break;
		}
	}
	k_mutex_unlock(&buf_read_mutex);

	LZ4F_preferences_t prefs = LZ4F_INIT_PREFERENCES;
	prefs.compressionLevel = LZ4HC_CLEVEL_MIN - 1;

	int compress_frame_bound = LZ4F_compressFrameBound(index, &prefs);
	LOG_DBG("compressFrameBound is %d", compress_frame_bound);
	if (compress_frame_bound > max_payload_size * compression_factor * 1.1)
	{
		LOG_ERR("Frame too big");
		return;
	}

	// Compress the whole joined data
	int compressed_size = LZ4F_compressFrame(compressed_data, (int)(max_payload_size * compression_factor * 1.1), joined_data, index, &prefs);
	// If the compressed size exceeds the payload size, abort send
	if (compressed_size <= 0)
	{
		LOG_ERR("Compression failed with error %d!", compressed_size);
		return;
	}
	else if (compressed_size > max_payload_size)
	{
		LOG_WRN("Compressed size bigger than payload, discarding...");
		return;
	}
	else
	{
		LOG_DBG("Compressed size: %d", compressed_size);

		error = lorawan_send(2, compressed_data, compressed_size, LORAWAN_MSG_UNCONFIRMED);
	}

#else

	error = ring_buf_item_get(&lorawan_internal_buffer, &type, &value, encoded_data, &encoded_data_size);
	if (error)
		goto buf_err;

	encoded_data_size *= 4;

	// Assume that size > 0 since no error happened right?
	char hex[256];
	int hexsize = bin2hex((uint8_t *)encoded_data, encoded_data_size, hex, 256);

	LOG_DBG("Sending binary data: \"%s\", with %d bytes", hex, encoded_data_size);

	// Send using Zephyr's subsystem and check if the transmission was successful
	error = lorawan_send(0, (uint8_t *)encoded_data, encoded_data_size, LORAWAN_MSG_UNCONFIRMED);

#endif

	if (error)
		LOG_ERR("lorawan_send failed: %d.", error);
	else
		LOG_INF("lorawan_send successful");

buf_err:

	if (error == -EAGAIN)
		LOG_INF("Internal lorawan buffer is empty!");
	else if (error)
		LOG_ERR("read from ring buf failed: %d.", error);
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