#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <communication/lora/lora_common.h>
#include <communication/lora/lora_device/lora_device.h>
#include <communication/lora/lora_p2p/lora_p2p_interface.h>
#include <integration/data_buffer/buffer_service.h>

LOG_MODULE_REGISTER(lora_p2p_interface, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Create an internal buffer to be able to send multiple data readings in one packet
#define LORA_P2P_BUFFER_SIZE 2048
// Defines the internal buffer for used to store data while waiting a previous package to be sent
RING_BUF_ITEM_DECLARE(lora_p2p_ring_buffer, LORA_P2P_BUFFER_SIZE);
PulgaRingBuffer lora_p2p_buffer;

static ChannelAPI lora_p2p_api;

// Stack of the thread that takes data read from general buffer and prepares them to send
static K_THREAD_STACK_DEFINE(lora_p2p_thread_stack_area, LORA_P2P_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread lora_p2p_thread_data;
static k_tid_t lora_p2p_thread_id;
// Stack of the thread that actually sends the data via Lora Peer-to-Peer
static K_THREAD_STACK_DEFINE(lora_p2p_send_thread_stack_area, LORA_P2P_SEND_THREAD_STACK_SIZE);
static struct k_thread lora_p2p_send_thread_data;
static k_tid_t lora_p2p_send_thread_id;

// Configures connection, initializes internal buffer and thread to send data via LoRa Peer-to-Peer
static int lora_p2p_init_channel(void);
// Function executed by thread that sends data using LoRa Peer-to-Peer
static void lora_p2p_send_data(void *, void *, void *);

/**
 * Definitions
 */

static int lora_p2p_init_channel(void)
{
	LOG_DBG("Initializing LoRa Peer-to-Peer channel");
	int error = 0;

	if (!lora_device.is_ready(&lora_device))
    {
        return -EAGAIN;
    }

	init_pulga_buffer(&lora_p2p_buffer, &lora_p2p_ring_buffer);

	error = lora_device.setup_lora_connection(&lora_device, LORA_P2P);
	if (error)
	{
		goto return_clause;
	}

	LOG_DBG("Initializing LoRa Peer-to-Peer processing data thread");
	// After joining successfully, create the send thread.
	lora_p2p_thread_id = k_thread_create(&lora_p2p_thread_data, lora_p2p_thread_stack_area,
										 K_THREAD_STACK_SIZEOF(lora_p2p_thread_stack_area),
										 lora_process_data, (void *)(uintptr_t)LORA_P2P, &lora_p2p_buffer, &lora_p2p_send_thread_id,
										 LORA_P2P_PROCESSING_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(lora_p2p_thread_id, "lora_p2p_process_data");
	if (error)
	{
		LOG_ERR("Failed to set LoRa Peer-to-Peer processing data thread name: %d", error);
		goto return_clause;
	}
	LOG_DBG("Initializing send via LoRa Peer-to-Peer thread");
	lora_p2p_send_thread_id = k_thread_create(&lora_p2p_send_thread_data, lora_p2p_send_thread_stack_area,
											  K_THREAD_STACK_SIZEOF(lora_p2p_send_thread_stack_area),
											  lora_p2p_send_data, NULL, NULL, NULL,
											  LORA_P2P_SEND_THREAD_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(lora_p2p_thread_id, "send_lora_p2p");
	if (error)
		LOG_ERR("Failed to set send via LoRa Peer-to-Peer thread name: %d", error);

return_clause:
	return error;
}

#ifdef CONFIG_LORA_P2P_JOIN_PACKET
void lora_p2p_send_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Sending via lora_p2p started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);

	int max_payload_size, available_package_size;
	uint8_t insert_index, error = 0;
	uint8_t joined_data[MAX_DATA_LEN];
	reset_join_variables(&max_payload_size, &insert_index, &available_package_size, joined_data, LORA_P2P);

	while (1)
	{
		// After waking up, transmits until buffer is empty
		while (!buffer_is_empty(&lora_p2p_buffer))
		{
			LOG_DBG("Resetting data item variables");
			error = 0;
			uint8_t encoded_data_word_size;
			uint32_t encoded_data[MAX_32_WORDS];
			memset(encoded_data, 0, sizeof(encoded_data));
			// Peeking the size of the next item in the buffer
			error = get_item_word_size(&lora_p2p_buffer, &encoded_data_word_size);
			if (error)
			{
				encoded_data_word_size = MAX_32_WORDS;
			}

			// Sends package as the new item wouldn't fit in it and resets package variables to form a new one
			if (available_package_size - SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size) < 0)
			{
				lora_device.send_package(&lora_device, LORA_P2P, (uint8_t *)joined_data, SIZE_32_BIT_WORDS_TO_BYTES(insert_index));
				reset_join_variables(&max_payload_size, &insert_index,
									 &available_package_size, joined_data, LORA_P2P);
				continue;
			}
			enum DataType data_type;
			// Get the next packet from the internal buffer
			error = get_from_buffer(&lora_p2p_buffer, encoded_data, &data_type, &encoded_data_word_size);
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
#else
static void lora_p2p_send_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Sending via lora-p2p");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	int error;

	while (1)
	{
		// After waking up, transmits until buffer is empty
		while (!buffer_is_empty(&lora_p2p_buffer))
		{
			LOG_DBG("Resetting data item variables");
			error = 0;
			uint8_t encoded_data_size, encoded_data_word_size;
			uint32_t encoded_data[MAX_32_WORDS];

			memset(encoded_data, 0, sizeof(encoded_data));
			encoded_data_word_size = MAX_32_WORDS;
			enum DataType data_type;
			// Get the next packet from the internal buffer
			error = get_from_buffer(&lora_p2p_buffer, encoded_data, &data_type, &encoded_data_word_size);
			if (error)
			{
				continue;
			}
			encoded_data_size = SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size);
			lora_device.send_package(&lora_device, LORA_P2P, (uint8_t *)encoded_data, encoded_data_size);
		}
		LOG_DBG("Buffer is empty, sleeping");
		k_sleep(K_FOREVER);
	}
}

#endif // CONFIG_LORA_P2P_JOIN_PACKET

ChannelAPI *register_lora_p2p_callbacks()
{
	LOG_DBG("Initializing lora p2p callbacks");
	lora_p2p_api.init_channel = lora_p2p_init_channel;
	return &lora_p2p_api;
}