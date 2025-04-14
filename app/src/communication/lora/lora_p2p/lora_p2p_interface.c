#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>
#include "lora_p2p_interface.h"

#if (CONFIG_LORA_P2P_CR == 5)
#define LORA_CR CR_4_5
#elif (CONFIG_LORA_P2P_CR == 6)
#define LORA_CR CR_4_6
#elif (CONFIG_LORA_P2P_CR == 7)
#define LORA_CR CR_4_7
#elif (CONFIG_LORA_P2P_CR == 8)
#define LORA_CR CR_4_8
#else
#error "Coding Rate between 5 and 8 must be chosen"
#endif

#if (CONFIG_LORA_P2P_BW == 125)
#define LORA_BW BW_125_KHZ
#elif (CONFIG_LORA_P2P_BW == 250)
#define LORA_BW BW_250_KHZ
#elif (CONFIG_LORA_P2P_BW == 500)
#define LORA_BW BW_500_KHZ
#else
#error "Bandwidth 125, 250 or 500 must be chosen"
#endif

#if (CONFIG_LORA_P2P_SF == 12)
#define LORA_SF SF_12
#elif (CONFIG_LORA_P2P_SF == 11)
#define LORA_SF SF_11
#elif (CONFIG_LORA_P2P_SF == 10)
#define LORA_SF SF_10
#elif (CONFIG_LORA_P2P_SF == 9)
#define LORA_SF SF_9
#elif (CONFIG_LORA_P2P_SF == 8)
#define LORA_SF SF_8
#elif (CONFIG_LORA_P2P_SF == 7)
#define LORA_SF SF_7
#elif (CONFIG_LORA_P2P_SF == 6)
#define LORA_SF SF_6
#else
#error "Spreading Factor between 6 and 12 must be chosen"
#endif

LOG_MODULE_REGISTER(lora_p2p_interface, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */
static ChannelAPI lora_p2p_api;
// Initializes LoRa Peer-to-Peer channel
static int lora_p2p_init_channel(void);
// Callback function to be used whenever data is received.
static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
					 int16_t rssi, int8_t snr, void *user_data);
// Function executed by thread that sends data using LoRa Peer-to-Peer
static void lora_p2p_send_data(void *__lora_dev__, void *param1, void *param2);
// Stack of lora_p2p communication thread
static K_THREAD_STACK_DEFINE(lora_p2p_thread_stack_area, LORA_P2P_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread lora_p2p_thread_data;
static k_tid_t lora_p2p_thread_id;
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
int toggle_reception_and_send(struct device *lora_device, uint8_t *payload_data, int payload_size);
#endif // CONFIG_RECEIVE_LORA_P2P

// Get the devicetree node for the LoRa hardware.
// It should have an alias declared as lora0 to be properly found!
#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
			 "No default LoRa radio specified in DT");

// Define the maximum string size that will be sent
#define MAX_DATA_LEN 256

static int lora_p2p_init_channel(void)
{
	// It is most important to get the device struct from the devicetree node
	// they are not the same thing!
	LOG_DBG("Setting up LoRa Peer-to-Peer connection");

	const struct device *lora_device;
	int error = 0;

	lora_device = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_device))
	{
		LOG_ERR("%s: device not ready.", lora_device->name);
		return -EAGAIN;
	}

	error = lora_config(lora_device, &lora_modem_config);
	if (error < 0)
	{
		LOG_ERR("lora_config failed");
		goto return_clause;
	}

	lora_recv_async(lora_device, lora_receive_cb, NULL);

	lora_p2p_thread_id = k_thread_create(&lora_p2p_thread_data, lora_p2p_thread_stack_area,
										 K_THREAD_STACK_SIZEOF(lora_p2p_thread_stack_area),
										 lora_p2p_send_data, (void *)lora_device, NULL, NULL,
										 LORA_P2P_THREAD_PRIORITY, 0, K_NO_WAIT);
	error = k_thread_name_set(lora_p2p_thread_id, "send_lora_p2p");
	if (error)
		LOG_ERR("Failed to set read buffer thread name: %d", error);

return_clause:
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


static void lora_p2p_send_data(void *__lora_dev__, void *param1, void *param2)
{
	LOG_INF("Sending via lora-p2p");
	ARG_UNUSED(param1);
	ARG_UNUSED(param2);

	struct device *lora_dev = (struct device *)__lora_dev__;

	while (1)
	{
		int size = -1, error;
		uint8_t encoded_data[MAX_DATA_LEN];

		// Waits for data to be ready
		k_sem_take(&data_ready_sem[LORA_P2P], K_FOREVER);

		// Encoding data to minimal string
		size = encode_data(data_unit.data_words, data_unit.data_type, MINIMALIST,
						   encoded_data, sizeof(encoded_data));
		if (size >= 0)
			LOG_DBG("Encoded data: \"%s\", size %d bytes", encoded_data, size);
		else
			LOG_ERR("Could not encode data");

#ifdef CONFIG_RECEIVE_LORA_P2P
		error = toggle_reception_and_send(lora_dev, encoded_data, size);
		if (error)
		{
			k_sem_give(&data_processed);
			continue;
		}
#else
		// Send using Zephyr's subsystem and check if the transmission was successful
		error = lora_send(lora_dev, encoded_data, size);
		if (error)
		{
			LOG_ERR("lorawan_send failed: %d.", error);
			k_sem_give(&data_processed);
			continue;
		}
		LOG_INF("lorawan_send successful");
#endif // CONFIG_RECEIVE_LORA_P2P

		// Signals back that sending is complete
		k_sem_give(&data_processed);
	}
}


#ifdef CONFIG_RECEIVE_LORA_P2P
int toggle_reception_and_send(struct device *lora_device, uint8_t *payload_data, int payload_size)
{
	int error = 0;

	// Stop receiving and configure the modem for transmission
	lora_recv_async(lora_device, NULL, NULL);
	lora_modem_config.tx = true;
	error = lora_config(lora_device, &lora_modem_config);
	if (error < 0)
	{
		LOG_ERR("lora_config failed");
		return error;
	}
	// Send using Zephyr's subsystem and check if the transmission was successful
	error = lora_send(lora_device, payload_data, payload_size);
	if (error < 0)
	{
		LOG_ERR("lora_send failed");
		return error;
	}
	LOG_INF("lora_send successful");
	// Reconfigure the modem for reception
	lora_modem_config.tx = false;
	error = lora_config(lora_device, &lora_modem_config);
	if (error < 0)
	{
		LOG_ERR("lora_config failed");
		return error;
	}
	lora_recv_async(lora_device, lora_receive_cb, NULL);
	return 0;
}
#endif // CONFIG_RECEIVE_LORA_P2P

ChannelAPI *register_lora_p2p_callbacks()
{
	LOG_DBG("Initializing lora p2p callbacks");
	lora_p2p_api.init_channel = lora_p2p_init_channel;
	return &lora_p2p_api;
}