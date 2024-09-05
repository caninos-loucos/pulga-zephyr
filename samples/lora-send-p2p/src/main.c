#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lora_send, CONFIG_APP_LOG_LEVEL);

// Get the devicetree node for the LoRa hardware.
// It should have an alias declared as lora0 to be properly found!
#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
	     "No default LoRa radio specified in DT");

// Get the devicetree node for the LED, that must be aliased as led0.
#define LED0_NODE DT_ALIAS(led0)

// The LED will be treated as a GPIO
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE);

// Define the string that will be sent each time
#define MAX_DATA_LEN 10
char data[MAX_DATA_LEN] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd'};

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
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 20;
	config.tx = true;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return 0;
	}

	// Send the predefined data string forever in an 1s interval
	while (1) {
		ret = lora_send(lora_dev, data, MAX_DATA_LEN);
		if (ret < 0) {
			LOG_ERR("LoRa send failed");
			return 0;
		}
		LOG_INF("Data sent!");

		k_sleep(K_MSEC(1000));
	}

	return 0;
}

