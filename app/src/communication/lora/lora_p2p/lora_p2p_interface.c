#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <communication/lora/lora_common.h>
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

// Initializes LoRa Peer-to-Peer channel
static int lora_p2p_init_channel(void);
// Callback function to be used whenever data is received.
static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
							int16_t rssi, int8_t snr, void *user_data);
// Function executed by thread that sends data using LoRa Peer-to-Peer
static void lora_p2p_send_data(void *, void *, void *);
// Functions that receives data from application buffer and
// inserts it in LoRa Peer-to-Peer internal buffer
static void lora_p2p_process_data(void *, void *, void *);

#ifdef CONFIG_LORA_P2P_JOIN_PACKET
// Resets variables used to join packets into package
static void reset_join_variables(int *max_payload_size, uint8_t *insert_index,
								 int *available_package_size, uint8_t *joined_data);
// Adds data item from buffer to package
static void add_item_to_package(uint8_t encoded_data_word_size, int max_payload_size,
								int *available_package_size, uint8_t *joined_data,
								uint8_t *insert_index, uint32_t *encoded_data);
#endif // CONFIG_LORA_P2P_JOIN_PACKET

// Configure the LoRa transmission parameters.
// Such parameters must match between the transmitter and
// receiver for communication to occur.
struct lora_modem_config lora_modem_config = {
	.frequency = 915000000,
	.bandwidth = LORA_BW,
	.datarate = LORA_SF,
	.preamble_len = 8,
	.coding_rate = LORA_CR,
	.iq_inverted = false,
	.public_network = false,
	.tx_power = 20,
	.tx = (CONFIG_SEND_LORA_P2P && !CONFIG_RECEIVE_LORA_P2P)};
#ifdef CONFIG_RECEIVE_LORA_P2P
int toggle_reception_and_send(uint8_t *payload_data, int payload_size);
#else
// Sends LoRa Peer-to-Peer package and handles errors
static void send_package(uint8_t *package, uint8_t package_size);
#endif // CONFIG_RECEIVE_LORA_P2P

// Define the maximum string size that will be sent
#define MAX_DATA_LEN 256

static int lora_p2p_init_channel(void)
{
	LOG_DBG("Initializing LoRa Peer-to-Peer channel");
	int error = 0;

	k_sem_take(lora_device.device_sem, K_FOREVER);
	error = !device_is_ready(lora_device.device);
	k_sem_give(lora_device.device_sem);
	if (error)
	{
		LOG_ERR("%s: device not ready.", lora_device.device->name);
		return -EAGAIN;
	}

	init_pulga_buffer(&lora_p2p_buffer, &lora_p2p_ring_buffer);

	error = setup_lora_p2p_connection();
	if (error)
	{
		LOG_ERR("Failed to setup LoRa Peer-to-Peer connection: %d", error);
		goto return_clause;
	}

	LOG_DBG("Initializing LoRa Peer-to-Peer processing data thread");
	// After joining successfully, create the send thread.
	lora_p2p_thread_id = k_thread_create(&lora_p2p_thread_data, lora_p2p_thread_stack_area,
										 K_THREAD_STACK_SIZEOF(lora_p2p_thread_stack_area),
										 lora_p2p_process_data, NULL, NULL, NULL,
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

int setup_lora_p2p_connection()
{
	LOG_DBG("Setting up LoRa Peer-to-Peer connection");
	int error = 0;

	k_sem_take(lora_device.device_sem, K_FOREVER);

	error = lora_config(lora_device.device, &lora_modem_config);
	if (error < 0)
	{
		LOG_ERR("lora_config failed");
		goto return_clause;
	}
#ifdef CONFIG_RECEIVE_LORA_P2P
	// Start reception
	error = lora_recv_async(lora_device.device, lora_receive_cb, NULL);
	if (error < 0)
	{
		LOG_ERR("Starting LoRa Peer-to-Peer reception failed");
		goto return_clause;
	}
#endif // CONFIG_RECEIVE_LORA_P2P
	LOG_INF("LoRa Peer-to-Peer connection setup complete");

return_clause:
	k_sem_give(lora_device.device_sem);
	return error;
}

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
					 int16_t rssi, int8_t snr, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(size);
	ARG_UNUSED(user_data);

	LOG_INF("LoRa RX RSSI: %d dBm, SNR: %d dB", rssi, snr);
	LOG_HEXDUMP_INF(data, size, "LoRa RX payload");
}

// Encoding and buffering Data thread
void lora_p2p_process_data(void *param0, void *param1, void *param2)
{
	LOG_INF("Processing Lora Peer-to-Peer data started");
	ARG_UNUSED(param0);
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);
	int error;

	while (1)
	{
		// Waits for data to be ready
		k_sem_take(&data_ready_sem[LORA_P2P], K_FOREVER);
		// Encodes data item to be sent and inserts the encoded data in the internal buffer
		error = encode_and_insert(&lora_p2p_buffer, data_unit, MINIMALIST);
		if (error)
		{
			k_sem_give(&data_processed);
			continue;
		}

#ifdef CONFIG_LORA_P2P_JOIN_PACKET
		// If the application is joining packets into a larger package,
		// it waits longer to wake up the sending thread, until a package with
		// maximum payload size can be assembled
		if (get_buffer_size_without_headers(&lora_p2p_buffer) < MAX_DATA_LEN)
		{
			LOG_DBG("Joining more data");
			// Signals for the Communication Interface that Lorawan processing is complete
			k_sem_give(&data_processed);
			continue;
		}
#endif // CONFIG_LORA_P2P_JOIN_PACKET
		LOG_DBG("Waking up sending thread");
		k_wakeup(lora_p2p_send_thread_id);
		// Signals for the Communication Interface that Lorawan processing is complete
		k_sem_give(&data_processed);
	}
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
	reset_join_variables(&max_payload_size, &insert_index, &available_package_size, joined_data);

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
#ifdef CONFIG_RECEIVE_LORA_P2P
				error = toggle_reception_and_send(joined_data, max_payload_size - available_package_size);
				if (error)
				{
					continue;
				}
#else
				send_package(joined_data, max_payload_size - available_package_size);
#endif // CONFIG_RECEIVE_LORA_P2P

				reset_join_variables(&max_payload_size, &insert_index,
									 &available_package_size, joined_data);
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

void reset_join_variables(int *max_payload_size, uint8_t *insert_index,
						  int *available_package_size, uint8_t *joined_data)
{
	LOG_DBG("Resetting join variables");
	*insert_index = 0;
	// Resetting join package variables after last send
	memset(joined_data, 0, 256);
	*max_payload_size = MAX_DATA_LEN;
	*available_package_size = *max_payload_size;
	LOG_DBG("Maximum payload size for current datarate: %d B", *available_package_size);
}

void add_item_to_package(uint8_t encoded_data_word_size, int max_payload_size,
						 int *available_package_size, uint8_t *joined_data,
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
#ifdef CONFIG_RECEIVE_LORA_P2P
			error = toggle_reception_and_send((uint8_t *)encoded_data, encoded_data_size);
			if (error)
			{
				continue;
			}
#else
			send_package((uint8_t *)encoded_data, encoded_data_size);
#endif // CONFIG_RECEIVE_LORA_P2P
		}
		LOG_DBG("Buffer is empty, sleeping");
		k_sleep(K_FOREVER);
	}
}

#endif // CONFIG_LORA_P2P_JOIN_PACKET

#ifdef CONFIG_RECEIVE_LORA_P2P
int toggle_reception_and_send(uint8_t *payload_data, int payload_size)
{
	int error = 0;

	k_sem_take(lora_device.device_sem, K_FOREVER);
	// Stop receiving and configure the modem for transmission
	lora_recv_async(lora_device.device, NULL, NULL);
	lora_modem_config.tx = true;
	error = lora_config(lora_device.device, &lora_modem_config);
	if (error < 0)
	{
		LOG_ERR("lora_config failed");
		goto return_clause;
	}
	k_sleep(K_SECONDS(2));
	// Send using Zephyr's subsystem and check if the transmission was successful
	error = lora_send(lora_device.device, payload_data, payload_size);
	if (error < 0)
	{
		LOG_ERR("lora_send failed");
		goto return_clause;
	}
	LOG_INF("lora_send successful");
	// Reconfigure the modem for reception
	lora_modem_config.tx = false;
	error = lora_config(lora_device.device, &lora_modem_config);
	if (error < 0)
	{
		LOG_ERR("lora_config failed");
		goto return_clause;
	}
	lora_recv_async(lora_device.device, lora_receive_cb, NULL);

return_clause:
	k_sem_give(lora_device.device_sem);
	return error;
}
#else
void send_package(uint8_t *package, uint8_t package_size)
{
	k_sem_take(lora_device.device_sem, K_FOREVER);
	// Send using Zephyr's subsystem and check if the transmission was successful
	int error = lora_send(lora_device.device, package, package_size);
	k_sem_give(lora_device.device_sem);
	if (error)
	{
		LOG_ERR("lora_p2p_send failed: %d.", error);
		return;
	}
	LOG_INF("lora_p2p_send successful");
}
#endif // CONFIG_RECEIVE_LORA_P2P

ChannelAPI *register_lora_p2p_callbacks()
{
	LOG_DBG("Initializing lora p2p callbacks");
	lora_p2p_api.init_channel = lora_p2p_init_channel;
	return &lora_p2p_api;
}