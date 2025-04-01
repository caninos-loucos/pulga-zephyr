/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lora_receive, CONFIG_APP_LOG_LEVEL);

// Get the devicetree node for the LoRa hardware.
// It should have an alias declared as lora0 to be properly found!
#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

// Get the devicetree node for the LED, that must be aliased as led0.
#define LED0_NODE DT_ALIAS(led0)

#define MAX_DATA_LEN 255

// The LED will be treated as a GPIO
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

// Receive callback, called everytime there is a packet reception.
// Will print the packet and reception parameters in the log and then blink the LED
void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
	                 int16_t rssi, int8_t snr, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(size);
	ARG_UNUSED(user_data);

	// RSSI: Received Signal Strength Indicator
	// SNR: Signal-noise ratio
	LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)",
		data, rssi, snr);

	gpio_pin_set_dt(&led, 1);
	k_msleep(200);
	gpio_pin_set_dt(&led, 0);

	return;
}

int main(void)
{
	// It is most important to get the device struct from the devicetree node
	// they are not the same thing!
	const struct device *lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

	// Declare the remaining variables since the C11 convention 
	// does not allow mixed declarations and statements
	struct lora_modem_config config;
	int ret;


	// Check, configure and initialize the LED with 0, respectively
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Could not setup the indicator LED");
		return 0;
	}

	gpio_pin_set_dt(&led, 0);

	// Check the LoRa device
	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

	// Configure the LoRa transmission parameters.
	// Such parameters must match between the transmitter and 
	// receiver for communication to occur.
	config.frequency = 915000000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_7;
	config.preamble_len = 8;
	config.coding_rate = CR_4_7;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx = false;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return 0;
	}

	// Enable asynchronous reception, so that the callback will
	// execute everytime the LoRa device sends an RX interruption
	LOG_INF("Starting reception...");
	lora_recv_async(lora_dev, lora_receive_cb, NULL);
	k_sleep(K_FOREVER);
	return 0;
}

