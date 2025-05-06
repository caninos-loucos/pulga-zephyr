#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <communication/lora/lora_device/lora_device.h>
#include <communication/lora/lora_device/lorawan_device/lorawan_device.h>
#include <integration/timestamp/timestamp_service.h>

/* 	Get the LoraWAN keys from file in main directory, in the following format:

    #define LORAWAN_DEV_EUI		{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01 }
    #define LORAWAN_APP_EUI  	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01 }
    #define LORAWAN_DEV_ADDR	0xabcdef01
    #define LORAWAN_NET_KEY 	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }
    #define LORAWAN_APP_KEY 	{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 }
*/
#include <communication/lora/lorawan/lorawan_keys_example.h>

LOG_MODULE_REGISTER(lorawan_device, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Configures lorawan connection, joining the network
static int setup_lorawan_connection(const struct device *lora_device, bool transm_enabled);
// Sends LoRaWAN package and handles errors
static int send_lorawan_package(const struct device *lora_device, uint8_t *package, int package_size);
#ifdef CONFIG_EVENT_TIMESTAMP_LORAWAN
// Sends a request to the network to get the current time and
// sets it as the synchronization time
static int get_network_time(bool force_request);
#endif // CONFIG_EVENT_TIMESTAMP_LORAWAN
// Set security configuration parameters for joining network
static inline void lorawan_config_activation(struct lorawan_join_config *join_config);

LoraDeviceAPI lorawan_device = {
    .send_package = send_lorawan_package,
#ifdef CONFIG_EVENT_TIMESTAMP_LORAWAN
    .sync_timestamp = get_network_time,
#else
    .sync_timestamp = NULL,
#endif // CONFIG_EVENT_TIMESTAMP_LORAWAN
    .acquire_device = setup_lorawan_connection,
    .release_device = NULL,
    .check_configuration = NULL,
};

/**
 * Definitions
 */

// Configures lorawan connection, joining the network
int setup_lorawan_connection(const struct device *lora_device, bool transm_enabled)
{
    LOG_DBG("Setting up LoRaWAN connection");
    ARG_UNUSED(lora_device);
    ARG_UNUSED(transm_enabled);
    int error = 0;

    // Configuration structure to join network
    struct lorawan_join_config join_config;
    lorawan_config_activation(&join_config);

    error = lorawan_join(&join_config);
    if (error)
    {
        LOG_ERR("lorawan_join_network failed: %d", error);
        goto return_clause;
    }
    LOG_DBG("LoRaWAN connection setup successful");
return_clause:
    return error;
}

// Set security configuration parameters for joining network
inline void lorawan_config_activation(struct lorawan_join_config *join_config)
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

#if defined(CONFIG_EVENT_TIMESTAMP_LORAWAN)
int get_network_time(bool force_request)
{
    int error = 0;
    uint32_t gps_epoch = 0;
    // Request network time
    error = lorawan_request_device_time(force_request);
    if (error)
    {
        LOG_ERR("Error requesting LoRaWAN network time: %d", error);
        goto return_clause;
    }
    error = lorawan_device_time_get(&gps_epoch);
    if (error)
    {
        LOG_ERR("Error getting LoRaWAN network time: %d", error);
        goto return_clause;
    }
    // Converts the GPS epoch to Unix epoch
    gps_epoch = GPS_EPOCH_TO_POSIX(gps_epoch);
    LOG_INF("LoRaWAN network time: %d", gps_epoch);
    // Sets the timestamp as the synchronization time
    set_sync_time_seconds(gps_epoch);

return_clause:
    return error;
}
#endif

int send_lorawan_package(const struct device *lora_device, uint8_t *package, int package_size)
{
    ARG_UNUSED(lora_device);
    // Send using Zephyr's subsystem and check if the transmission was successful
    int error = lorawan_send(1, package, package_size, LORAWAN_MSG_UNCONFIRMED);
    if (error)
    {
        LOG_ERR("lorawan_send failed: %d.", error);
    }
    else
    {
        LOG_DBG("lorawan_send successful");
    }
    return error;
}