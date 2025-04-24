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
#include <communication/lora/lora_common.h>
#include <communication/lora/lora_device/lora_device.h>
#include <communication/lora/lorawan/lorawan_interface.h>
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
											 lora_send_data, (void *)(uintptr_t) LORAWAN, &lorawan_buffer, NULL,
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

bool is_lorawan_buffer_empty()
{
	return buffer_is_empty(&lorawan_buffer);
}

// Register channels to the Communication Module
ChannelAPI *register_lorawan_callbacks()
{
	LOG_DBG("Initializing lorawan callbacks");
	lorawan_api.init_channel = lorawan_init_channel;
	return &lorawan_api;
}