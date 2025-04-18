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
#include <communication/lora/lorawan/lorawan_interface.h>
#include <communication/lora/lora_common.h>
#include <integration/data_buffer/buffer_service.h>

LOG_MODULE_REGISTER(lorawan_interface, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Create an internal buffer to be able to send multiple data readings in one packet
#define LORAWAN_BUFFER_SIZE 2048
// Defines the internal buffer for used to store data while waiting a previous package to be sent
RING_BUF_ITEM_DECLARE(lorawan_ring_buffer, LORAWAN_BUFFER_SIZE);
PulgaRingBuffer lorawan_buffer;

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
static int lorawan_init_channel();
// Sends LoRaWAN package and handles errors
static void send_package(uint8_t *package, uint8_t package_size);
// This is the function executed by the thread that actually sends the data
static void lorawan_send_data(void *, void *, void *);

/**
 * Definitions
 */

static int lorawan_init_channel()
{
	LOG_DBG("Initializing LoRaWAN channel");
	int error = 0;

	init_pulga_buffer(&lorawan_buffer, &lorawan_ring_buffer);

	error = init_lorawan_connection();
	if (error)
	{
		LOG_ERR("Failed to setup LoRaWAN connection: %d", error);
		goto return_clause;
	}

	LOG_DBG("Initializing LoRaWAN processing data thread");
	// After joining successfully, create the send thread.
	lorawan_thread_id = k_thread_create(&lorawan_thread_data, lorawan_thread_stack_area,
										K_THREAD_STACK_SIZEOF(lorawan_thread_stack_area),
										lora_process_data, (void *)(uintptr_t) LORAWAN, &lorawan_buffer, &lorawan_send_thread_id,
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
	return error;
}

#ifdef CONFIG_LORAWAN_JOIN_PACKET
void lorawan_send_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Sending via lorawan started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);

	int max_payload_size, available_package_size;
	uint8_t insert_index, error = 0;
	// Maximum LoRaWAN package size won't surpass 256 B
	uint8_t joined_data[256];
	reset_join_variables(&max_payload_size, &insert_index, &available_package_size, joined_data, LORAWAN);

	while (1)
	{
		// After waking up, transmits until buffer is empty
		while (!buffer_is_empty(&lorawan_buffer))
		{
			LOG_DBG("Resetting data item variables");
			error = 0;
			uint8_t encoded_data_word_size;
			uint32_t encoded_data[MAX_32_WORDS];
			memset(encoded_data, 0, sizeof(encoded_data));
			// Peeking the size of the next item in the buffer
			error = get_item_word_size(&lorawan_buffer, &encoded_data_word_size);
			if (error)
			{
				encoded_data_word_size = MAX_32_WORDS;
			}

			// Sends package as the new item wouldn't fit in it and resets package variables to form a new one
			if (available_package_size - SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size) < 0)
			{
				send_package(joined_data, max_payload_size - available_package_size);
				reset_join_variables(&max_payload_size, &insert_index,
									 &available_package_size, joined_data, LORAWAN);
				continue;
			}
			enum DataType data_type;
			// Get the next packet from the internal buffer
			error = get_from_buffer(&lorawan_buffer, encoded_data, &data_type, &encoded_data_word_size);
			if (error)
			{
				continue;
			}
			add_item_to_package(encoded_data_word_size, max_payload_size,
								&available_package_size, joined_data,
								&insert_index, encoded_data);
		}
		LOG_DBG("Buffer is empty, sleeping");
		k_sleep(K_FOREVER);
	}
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
		while (!buffer_is_empty(&lorawan_buffer))
		{
			LOG_DBG("Resetting data item variables");
			error = 0;
			uint8_t encoded_data_size, encoded_data_word_size;
			uint32_t encoded_data[MAX_32_WORDS];

			memset(encoded_data, 0, sizeof(encoded_data));
			encoded_data_word_size = MAX_32_WORDS;
			enum DataType data_type;
			// Get the next packet from the internal buffer
			error = get_from_buffer(&lorawan_buffer, encoded_data, &data_type, &encoded_data_word_size);
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
	// Waits for access to the LoRa device
	k_sem_take(lora_device.device_sem, K_FOREVER);
	// Send using Zephyr's subsystem and check if the transmission was successful
	int error = lorawan_send(1, package, package_size, LORAWAN_MSG_UNCONFIRMED);
	k_sem_give(lora_device.device_sem);
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