#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <communication/lora/lora_common.h>
#include <communication/lora/lora_device/lora_device.h>
#include <communication/lora/lorawan/lorawan_interface.h>

LOG_MODULE_REGISTER(lorawan_setup, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Downlink callback, dumps the received data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
static void downlink_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr, uint8_t length,
                              const uint8_t *hex_data);
// Callback to be used whenever datarate changes. When CONFIG_LORAWAN_COMPRESS_PACKET is set, adjusts the expected compression level.
static void dr_changed_callback(enum lorawan_datarate dr);
// Initialize the downlink callback
static struct lorawan_downlink_cb downlink_cb;

#if defined(CONFIG_EVENT_TIMESTAMP_LORAWAN)
// Handles requesting network time and retrying if it fails
static void sync_work_handler(struct k_work *work);
// Work to be queued and executed when the delay expires
static struct k_work_delayable sync_work;
static K_WORK_DELAYABLE_DEFINE(sync_work, sync_work_handler);
// Period to request network time again as a kernel timeout value
#define SYNC_PERIOD K_SECONDS(86400) // Once a day
#endif // CONFIG_EVENT_TIMESTAMP_LORAWAN

/**
 * Definitions
 */

// Checks readiness of the LoRa device, configures lorawan backend and timestamp synchronization
int init_lorawan_connection()
{
    LOG_DBG("Initializing LoRaWAN connection");
    int error = 0;

    if (!lora_device.is_ready())
    {
        return -EAGAIN;
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

#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
    error = acquire_ownership(LORAWAN, true);
#else
    error = lora_device.setup_lora_connection(LORAWAN, true);
#endif
    if (error)
    {
        goto return_clause;
    }
#if defined(CONFIG_EVENT_TIMESTAMP_LORAWAN)
    // Request network time until it succeeds and schedule periodic requests
    do
    {
        error = lora_device.sync_timestamp(LORAWAN, true);
    } while (error);
    k_work_schedule(&sync_work, SYNC_PERIOD);
#endif

// Releases the ownership of the LoRa device so P2P can also initilize
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
    error = lora_device.release_device(LORAWAN);
#endif

return_clause:
    return error;
}

// Downlink callback, dumps the received data onto the log as debug, along with several reception parameters:
// RSSI: Received Signal Strength Indicator
// SNR: Signal-noise ratio
void downlink_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr, uint8_t length,
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

#if defined(CONFIG_EVENT_TIMESTAMP_LORAWAN)
void sync_work_handler(struct k_work *work)
{
    int error = 0;
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
    error = acquire_ownership(LORAWAN, true);
    if (error)
    {
        k_work_schedule(&sync_work, K_SECONDS(30));
        return;
    }
#endif
    error = lora_device.sync_timestamp(LORAWAN, false);
    // Retry after some time if it fails
    if (error)
    {
        LOG_DBG("Trying to get LoRaWAN network time again");
        k_work_schedule(&sync_work, K_SECONDS(30));
        return;
    }
    // Schedule next request
    k_work_schedule(&sync_work, SYNC_PERIOD);
}
#endif
