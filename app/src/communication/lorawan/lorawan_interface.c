/*
Lorawan Interface Module
Authors: Ana Clara Forcelli <ana.forcelli@lsitec.org.br>
		 Matheus de Almeida Orsi e Silva <matheus.almeida.silva@alumni.usp.br>

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
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <communication/lorawan/lorawan_interface.h>
#include <communication/lorawan/lorawan_buffer/lorawan_buffer.h>

LOG_MODULE_REGISTER(lorawan_interface, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Instance of ChannelAPI for lorawan channel
static ChannelAPI lorawan_api;

// Stack of the thread that takes data read from general buffer and prepares them to send
static K_THREAD_STACK_DEFINE(lorawan_thread_stack_area, LORAWAN_PROCESSING_STACK_SIZE);
// Thread control block - metadata
static struct k_thread lorawan_thread_data;
static k_tid_t lorawan_thread_id;
// Stack of the thread that actually sends the data via LoRaWAN
static K_THREAD_STACK_DEFINE(lorawan_send_thread_stack_area, LORAWAN_SEND_THREAD_STACK_SIZE);
static struct k_thread lorawan_send_thread_data;
static k_tid_t lorawan_send_thread_id;

// Initializes and starts thread to send data via LoRaWAN
static void lorawan_init_channel();
// Functions that receives data from application buffer and
// inserts it in LoRaWAN internal buffer
static void lorawan_process_data(void *, void *, void *);
// Sends LoRaWAN package and handles errors
static void send_package(uint8_t *package, uint8_t package_size);
// This is the function executed by the thread that actually sends the data
static void lorawan_send_data(void *, void *, void *);
#ifdef CONFIG_LORAWAN_JOIN_PACKET
// Resets variables used to join packets into package
static void reset_join_variables(uint8_t *max_payload_size, uint8_t *insert_index,
								 uint8_t *available_package_size, uint8_t *joined_data);
// Adds data item from buffer to package
static void add_item_to_package(uint8_t encoded_data_word_size, uint8_t max_payload_size,
								uint8_t *available_package_size, uint8_t *joined_data,
								uint8_t *insert_index, uint32_t *encoded_data);
#endif // CONFIG_LORAWAN_JOIN_PACKET

/**
 * Definitions
 */

static void lorawan_init_channel()
{
	LOG_DBG("Initializing LoRaWAN channel");
	int error = 0;

	error = lorawan_setup_connection();

	LOG_DBG("Initializing LoRaWAN processing data thread");
	// After joining successfully, create the send thread.
	lorawan_thread_id = k_thread_create(&lorawan_thread_data, lorawan_thread_stack_area,
										K_THREAD_STACK_SIZEOF(lorawan_thread_stack_area),
										lorawan_process_data, NULL, NULL, NULL,
										LORAWAN_PROCESSING_PRIORITY, 0, K_NO_WAIT);
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
	error = k_thread_name_set(lorawan_send_thread_id, "lorawan_send_data");
	if (error)
	{
		LOG_ERR("Failed to set send via LoRaWAN thread name: %d", error);
		goto return_clause;
	}

return_clause:
	return;
}

int buffered_items = 0;
// Encoding and buffering Data thread
void lorawan_process_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Processing LoRaWAN data started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	uint8_t max_payload_size;
	uint8_t unused_arg;

	int error = 0;

	while (1)
	{
		// Waits for data to be ready
		k_sem_take(&data_ready_sem[LORAWAN], K_FOREVER);
		// Maximum payload size determined by datarate and region
		lorawan_get_payload_sizes(&unused_arg, &max_payload_size);
		// Encodes data item to be sent and inserts the encoded data in the internal buffer
		error = encode_and_insert(data_unit);
		if (error)
			continue;
		else
			buffered_items++;

#ifdef CONFIG_LORAWAN_JOIN_PACKET
		// If the application is joining packets into a larger package,
		// it waits longer to wake up the sending thread, until a package with
		// maximum payload size can be assembled
		if (get_buffer_to_package_size(buffered_items) < max_payload_size)
		{
			LOG_DBG("Joining more data");
			// Signals for the Communication Interface that Lorawan processing is complete
			k_sem_give(&data_processed);
			continue;
		}
#endif
		LOG_DBG("Waking up sending thread");
		k_wakeup(lorawan_send_thread_id);
		// Signals for the Communication Interface that Lorawan processing is complete
		k_sem_give(&data_processed);
	}
}

#ifdef CONFIG_LORAWAN_JOIN_PACKET
void lorawan_send_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Sending via lorawan started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	
	uint8_t max_payload_size, insert_index, available_package_size, error = 0;
	// Maximum LoRaWAN package size won't surpass 256 B
	uint8_t joined_data[256];
	reset_join_variables(&max_payload_size, &insert_index, &available_package_size, joined_data);
	
	while (1)
	{
		// After waking up, transmits until buffer is empty
		while (!lorawan_buffer_empty())
		{
			LOG_DBG("Resetting data item variables");
			error = 0;
			uint8_t encoded_data_word_size;
			uint32_t encoded_data[MAX_32_WORDS];
			memset(encoded_data, 0, sizeof(encoded_data));
			// Peeking the size of the next item in the buffer
			error = get_item_word_size(&encoded_data_word_size);
			if (error)
			{
				encoded_data_word_size = MAX_32_WORDS;
			}

			// Sends package as the new item wouldn't fit in it and resets package variables to form a new one
			if (available_package_size - SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size) < 0)
			{
				send_package(joined_data, max_payload_size - available_package_size);
				reset_join_variables(&max_payload_size, &insert_index,
									 &available_package_size, joined_data);
				continue;
			}
			// Get the next packet from the internal buffer
			error = get_lorawan_item(encoded_data, &encoded_data_word_size);
			if (error)
			{
				continue;
			}
			buffered_items--;
			add_item_to_package(encoded_data_word_size, max_payload_size,
								&available_package_size, joined_data,
								&insert_index, encoded_data);
		}
		LOG_DBG("Buffer is empty, sleeping");
		k_sleep(K_FOREVER);
	}
}

void reset_join_variables(uint8_t *max_payload_size, uint8_t *insert_index,
						  uint8_t *available_package_size, uint8_t *joined_data)
{
	LOG_DBG("Resetting join variables");
	uint8_t unused_arg;
	*insert_index = 0;
	// Resetting join package variables after last send
	memset(joined_data, 0, 256);
	lorawan_get_payload_sizes(&unused_arg, max_payload_size);
	*available_package_size = *max_payload_size;
	LOG_DBG("Maximum payload size for current datarate: %d B", *available_package_size);
}

void add_item_to_package(uint8_t encoded_data_word_size, uint8_t max_payload_size,
						 uint8_t *available_package_size, uint8_t *joined_data,
						 uint8_t *insert_index, uint32_t *encoded_data)
{
	uint8_t encoded_data_size = SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size);
	// Adds packet to package
	LOG_DBG("Adding item with size %d B to package with %d available bytes",
			encoded_data_size, *available_package_size);
	*insert_index = max_payload_size - *available_package_size;
	bytecpy(joined_data + *insert_index, encoded_data, encoded_data_size);
	*available_package_size -= encoded_data_size;
}
#else // CONFIG_LORAWAN_JOIN_PACKET
void lorawan_send_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Sending via lorawan started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	int error;

	while (1)
	{
		// After waking up, transmits until buffer is empty
		while (!lorawan_buffer_empty())
		{
			LOG_DBG("Resetting data item variables");
			error = 0;
			uint8_t encoded_data_size, encoded_data_word_size;
			uint32_t encoded_data[MAX_32_WORDS];

			memset(encoded_data, 0, sizeof(encoded_data));
			encoded_data_word_size = MAX_32_WORDS;
			// Get the next packet from the internal buffer
			error = get_lorawan_item(encoded_data, &encoded_data_word_size);
			if (error)
			{
				continue;
			}
			encoded_data_size = SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size);
			send_package((uint8_t *)encoded_data, encoded_data_size);
		}
		LOG_DBG("Buffer is empty, sleeping");
		k_sleep(K_FOREVER);
	}
}
#endif // CONFIG_LORAWAN_JOIN_PACKET

void send_package(uint8_t *package, uint8_t package_size)
{
	int error = lorawan_send(1, package, package_size, LORAWAN_MSG_UNCONFIRMED);
	// Send using Zephyr's subsystem and check if the transmission was successful
	if (error)
	{
		LOG_ERR("lorawan_send failed: %d.", error);
		return;
	}
	LOG_INF("lorawan_send successful");
}

// Register channels to the Communication Module
ChannelAPI *register_lorawan_callbacks()
{
	LOG_DBG("Initializing lorawan callbacks");
	lorawan_api.init_channel = lorawan_init_channel;
	return &lorawan_api;
}