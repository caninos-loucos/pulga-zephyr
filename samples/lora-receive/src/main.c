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

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

#define LED0_NODE DT_ALIAS(led0)

#define MAX_DATA_LEN 255

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_receive);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
	                 int16_t rssi, int8_t snr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(size);

	LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)",
		data, rssi, snr);

	/* Toggle the LED for 200ms to indicate a reception */
	gpio_pin_set_dt(&led, 1);
	k_msleep(200);
	gpio_pin_set_dt(&led, 0);

	return;
}

int main(void)
{
	const struct device *lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
	struct lora_modem_config config;
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Could not setup the indicator LED");
		return 0;
	}

	gpio_pin_set_dt(&led, 0);

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

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

	/* Enable asynchronous reception */
	LOG_INF("Starting reception...");
	lora_recv_async(lora_dev, lora_receive_cb);
	k_sleep(K_FOREVER);
	return 0;
}

