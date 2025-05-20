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
k_tid_t lora_p2p_send_thread_id = NULL;

// Configures connection, initializes internal buffer and thread to send data via LoRa Peer-to-Peer
static int lora_p2p_init_channel(void);
// Sends data via LoRa Peer-to-Peer
static void lora_p2p_send_data(void *channel, void *buffer, void *param2);
// Acquires LoRa device if necessary and sends the package
int acquire_and_send(enum ChannelType caller_channel, uint8_t *package, uint8_t package_size);

/**
 * Definitions
 */

static int lora_p2p_init_channel(void)
{
	LOG_DBG("Initializing LoRa Peer-to-Peer channel");
	int error = 0;

	if (!lora_device.is_ready())
	{
		return -EAGAIN;
	}

	init_pulga_buffer(&lora_p2p_buffer, &lora_p2p_ring_buffer);

	bool transm_enabled = IS_ENABLED(CONFIG_SEND_LORA_P2P) && !IS_ENABLED(CONFIG_RECEIVE_LORA_P2P);
	error = acquire_ownership(LORA_P2P, transm_enabled);
	if (error)
	{
		goto return_clause;
	}
	do
	{
		error = release_ownership(LORA_P2P);
	} while (error);

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
											  lora_p2p_send_data, (void *)(uintptr_t)LORA_P2P, &lora_p2p_buffer, NULL,
											  LORA_P2P_SEND_THREAD_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(lora_p2p_thread_id, "send_lora_p2p");
	if (error)
		LOG_ERR("Failed to set send via LoRa Peer-to-Peer thread name: %d", error);

return_clause:
	return error;
}

void lora_p2p_send_data(void *channel, void *buffer, void *param2)
{
	ARG_UNUSED(param2);

	enum ChannelType channel_type = (enum ChannelType)(uintptr_t)channel;
	PulgaRingBuffer *pulga_buffer = (PulgaRingBuffer *)buffer;
	LOG_INF("CHANNEL %d - Sending via lora started", channel_type);

#if IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET)
	struct join_variables join_vars;
	reset_join_variables(&join_vars, channel_type);
#endif // Any of the two join packets configurations

	while (1)
	{
		int error = 0;
		do
		{
			error = acquire_ownership(channel_type, false);
		} while (error);
		// Allows the LoRaWAN channel to interrupt the LoRa P2P channel
		do
		{
			error =  release_ownership(channel_type);
		} while (error);
		LOG_DBG("CHANNEL %d - Buffer is empty, sleeping", channel_type);
		k_sleep(K_FOREVER);
		// After waking up, transmits until buffer is empty
		while (!buffer_is_empty(pulga_buffer))
		{
			LOG_DBG("CHANNEL %d - Resetting data item variables", channel_type);
			error = 0;
			uint8_t encoded_data_word_size = MAX_32_WORDS;
			uint32_t encoded_data[MAX_32_WORDS];
			memset(encoded_data, 0, sizeof(encoded_data));

#if IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET)
			// Peeking the size of the next item in the buffer
			error = get_item_word_size(pulga_buffer, &encoded_data_word_size);
			// Sends package as the new item wouldn't fit in it and resets package variables to form a new one
			if (join_vars.available_package_size - SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size) < 0)
			{
				acquire_and_send(channel_type, join_vars.joined_data,
								 join_vars.max_payload_size - join_vars.available_package_size);
				reset_join_variables(&join_vars, channel_type);
				continue;
			}
#endif

			enum DataType data_type;
			// Get the next packet from the internal buffer
			error = get_from_buffer(pulga_buffer, encoded_data, &data_type, &encoded_data_word_size);
			if (error)
			{
				continue;
			}

#if IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET)
			add_item_to_package(&join_vars, encoded_data, encoded_data_word_size);
			continue;
#endif

			// Sends the packet directly if not joining
			acquire_and_send(channel_type, (uint8_t *)encoded_data,
							 SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size));
		}
		// Release after sending everything on buffer
		do
		{
			error = release_ownership(channel_type);
		} while (error);
	}
}

int acquire_and_send(enum ChannelType caller_channel, uint8_t *package, uint8_t package_size)
{
try_again:
	int error = 0;
	do
	{
		error = acquire_ownership(caller_channel, true);
	} while (error);
	error = lora_device.send_package(caller_channel, package, package_size);
	if (error)
	{
		LOG_ERR("CHANNEL %d - Failed to send package: %d", caller_channel, error);
		if(error == -EINVAL)
		{
			goto try_again;
		}
		goto return_clause;
	}
return_clause:
	return error;
}

ChannelAPI *register_lora_p2p_callbacks()
{
	LOG_DBG("Initializing lora p2p callbacks");
	lora_p2p_api.init_channel = lora_p2p_init_channel;
	return &lora_p2p_api;
}