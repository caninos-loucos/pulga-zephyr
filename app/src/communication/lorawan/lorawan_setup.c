#include <zephyr/device.h>
#include <zephyr/logging/log.h>

/* 	Get the LoraWAN keys from file in main directory, in the following format:

    #define LORAWAN_DEV_EUI		{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01 }
    #define LORAWAN_APP_EUI  	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01 }
    #define LORAWAN_DEV_ADDR	0xabcdef01
    #define LORAWAN_NET_KEY 	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }
    #define LORAWAN_APP_KEY 	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }
*/
#include <communication/lorawan/lorawan_keys_example.h>
#include <communication/lorawan/lorawan_interface.h>

LOG_MODULE_REGISTER(lorawan_setup, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Downlink callback, dumps the received data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
static void downlink_callback(uint8_t port, bool flags, int16_t rssi, int8_t snr, uint8_t length,
                              const uint8_t *hex_data);
// Callback to be used whenever datarate changes. When CONFIG_LORAWAN_COMPRESS_PACKET is set, adjusts the expected compression level.
static void dr_changed_callback(enum lorawan_datarate dr);
// Set security configuration parameters for joining network
static void lorawan_config_activation(struct lorawan_join_config *join_config);

// Initialize the callback
static struct lorawan_downlink_cb downlink_cb;

/**
 * Definitions
 */

// Configures and initializes lorawan connection, joining the network
int lorawan_setup_connection()
{
    LOG_DBG("Setting up LoRaWAN connection");

    const struct device *lora_device;
    int error = 0;

    lora_device = DEVICE_DT_GET(DT_ALIAS(lora0));
    if (!device_is_ready(lora_device))
    {
        LOG_ERR("%s: device not ready.", lora_device->name);
        goto return_clause;
    }

    // Set the chosen region, this MUST be the same as the gateway's!
    error = lorawan_set_region(LORAWAN_SELECTED_REGION);
    if (error)
    {
        LOG_ERR("lorawan_set_region failed: %d", error);
        goto return_clause;
    }

    // Register the callbacks before joining the network
    downlink_cb.port = LW_RECV_PORT_ANY;
    downlink_cb.cb = downlink_callback;
    lorawan_register_downlink_callback(&downlink_cb);
    lorawan_register_dr_changed_callback(dr_changed_callback);

    // Starts the LoRaWAN backend, transmission does not start just yet
    error = lorawan_start();
    if (error)
    {
        LOG_ERR("lorawan_start failed: %d", error);
        goto return_clause;
    }

    // Set the initial or fixed datarate according to the config
    error = lorawan_set_datarate(LORAWAN_DR);
    if (error)
    {
        LOG_ERR("lorawan_set_datarate failed: %d", error);
        goto return_clause;
    }

    // Configuration structure to join network
    struct lorawan_join_config join_config;
    lorawan_config_activation(&join_config);
    error = lorawan_join(&join_config);
    if (error)
    {
        LOG_ERR("lorawan_join_network failed: %d", error);
        goto return_clause;
    }
return_clause:
    return error;
}

// Downlink callback, dumps the received data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
void downlink_callback(uint8_t port, bool flags, int16_t rssi, int8_t snr, uint8_t length,
                       const uint8_t *hex_data)
{
    LOG_DBG("Port %d, Flags %x, RSSI %ddBm, SNR %ddB", port, flags, rssi, snr);
    if (hex_data)
    {
        LOG_HEXDUMP_INF(hex_data, length, "Payload: ");
    }
}

// Callback invoked everytime the datarate changes
void dr_changed_callback(enum lorawan_datarate new_dr)
{
    LOG_INF("Datarate changed to DR_%d", (int)new_dr);
}

// Set security configuration parameters for joining network
void lorawan_config_activation(struct lorawan_join_config *join_config)
{
    // Initialize the security parameters from the defined values
    static uint8_t device_id[] = LORAWAN_DEV_EUI;
    static uint8_t application_key[] = LORAWAN_APP_KEY;
    static uint8_t network_key[] = LORAWAN_NET_KEY;
    static uint8_t application_id[] = LORAWAN_APP_EUI;
    static uint32_t device_address = LORAWAN_DEV_ADDR;

    join_config->dev_eui = device_id;
#if defined(CONFIG_LORAWAN_OTAA)
    join_config->mode = LORAWAN_ACT_OTAA;
    join_config->otaa.join_eui = application_id;
    join_config->otaa.app_key = application_key;
    join_config->otaa.nwk_key = network_key;
    join_config->otaa.dev_nonce = 0u;
    LOG_DBG("Joining network over OTAA");
#elif defined(CONFIG_LORAWAN_ABP)
    join_config->mode = LORAWAN_ACT_ABP;
    join_config->abp.app_eui = application_id;
    join_config->abp.app_skey = application_key;
    join_config->abp.nwk_skey = network_key;
    join_config->abp.dev_addr = device_address;
    LOG_DBG("Joining network over ABP");
#endif
}
