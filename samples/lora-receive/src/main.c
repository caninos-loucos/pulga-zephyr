#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lora_receive, CONFIG_APP_LOG_LEVEL);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
#define MAX_DATA_LEN 255

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
	                 int16_t rssi, int8_t snr)
{
	static int cnt;

	ARG_UNUSED(dev);
	ARG_UNUSED(size);

	LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)",
		data, rssi, snr);

	/* Stop receiving after 10 packets */
	if (++cnt == 10) {
		LOG_INF("Stopping packet receptions");
		lora_recv_async(dev, NULL);
	}
}

int main(void)
{
	const struct device *lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
	struct lora_modem_config config;
	int ret, len;
	uint8_t data[MAX_DATA_LEN] = {0};
	int16_t rssi;
	int8_t snr;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

	config.frequency = 915000000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_7;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 20;
	config.tx = false;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return 0;
	}

	/* Receive 4 packets synchronously */
	LOG_INF("Synchronous reception");
	for (int i = 0; i < 4; i++) {
		/* Block until data arrives */
		len = lora_recv(lora_dev, data, MAX_DATA_LEN, K_FOREVER,
				&rssi, &snr);
		if (len < 0) {
			LOG_ERR("LoRa receive failed");
			return 0;
		}

		LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)",
			data, rssi, snr);
	}

	/* Enable asynchronous reception */
	LOG_INF("Asynchronous reception");
	lora_recv_async(lora_dev, lora_receive_cb);
	k_sleep(K_FOREVER);
	return 0;
}

