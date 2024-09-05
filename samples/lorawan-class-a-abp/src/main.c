/*
 * Class A LoRaWAN sample application
 *
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>

/* Customize based on network configuration */
/* 	#define LORAWAN_DEV_EUI
	#define LORAWAN_JOIN_EUI		
	#define LORAWAN_DEV_ADDR		
	#define LORAWAN_NET_KEY			
	#define LORAWAN_APP_KEY	*/
#include "keys.h"

#define LORAWAN_SELECTED_REGION LORAWAN_REGION_AU915

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_class_a);

char data[] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};

#define DELAY K_MSEC(1000)

// Downlink callback, dumps the receiving data onto the log, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
static void dl_callback(uint8_t port, bool data_pending,
			int16_t rssi, int8_t snr,
			uint8_t len, const uint8_t *hex_data)
{
	LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm", port, data_pending, rssi, snr);
	if (hex_data) {
		LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
	}
}

// Callback used for changes in data rate (ordered by the gateway)
static void lorawan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

int main(void)
{
	const struct device *lora_dev;
	struct lorawan_join_config join_cfg;
	int ret;

	// Initialize the security parameters from the defined values
	uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	uint8_t app_key[] = LORAWAN_APP_KEY;
	uint8_t nwk_key[] = LORAWAN_NET_KEY;
	uint8_t app_eui[] = LORAWAN_JOIN_EUI;
	uint32_t dev_addr = LORAWAN_DEV_ADDR;

	// Initialize the callback
	struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = dl_callback
	};

	// Get the LoRa device from the devicetree node
	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return 0;
	}

	// Set the chosen region, this MUST be the same as the gateway's!
	ret = lorawan_set_region(LORAWAN_SELECTED_REGION);
	if (ret < 0) {
		LOG_ERR("lorawan_set_region failed: %d", ret);
		return 0;
	}
	
	// Starts the LoRaWAN backend, transmission does not start just yet
	ret = lorawan_start();
	if (ret < 0) {
		LOG_ERR("lorawan_start failed: %d", ret);
		return 0;
	}

	// Register the callbacks before joining the network
	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorawan_datarate_changed);

	// Configure the authentication parameters and join the network
	join_cfg.mode = LORAWAN_ACT_ABP;
	join_cfg.dev_eui 		= dev_eui;
	join_cfg.abp.app_skey 	= app_key;
	join_cfg.abp.nwk_skey 	= nwk_key;
	join_cfg.abp.app_eui	= app_eui;
	join_cfg.abp.dev_addr 	= dev_addr;

	LOG_INF("Joining network over abp");
	ret = lorawan_join(&join_cfg);
	if (ret < 0) {
		LOG_ERR("lorawan_join_network failed: %d", ret);
		return 0;
	}

	// Send data indefinitely every DELAY milliseconds
	LOG_INF("Sending data...");
	while (1) {
		// Send the data to port 2. This can also take LORAWAN_MSG_UNCONFIRMED
		// to inform the gateway that it does not need to receive an ACK.
		ret = lorawan_send(2, data, sizeof(data),
				   LORAWAN_MSG_CONFIRMED);

		// In case of transmission errors, retransmit after DELAY.
		if (ret == -EAGAIN || ret == -ETIMEDOUT) {
			LOG_ERR("lorawan_send failed: %d. Continuing...", ret);
			k_sleep(DELAY);
			continue;
		}

		// If another error occurs, exit.
		if (ret < 0) {
			LOG_ERR("lorawan_send failed: %d", ret);
			return 0;
		}
 
		// if transmission is successful, do it again.
		LOG_INF("Data sent!");
		k_sleep(DELAY);
	}
}
