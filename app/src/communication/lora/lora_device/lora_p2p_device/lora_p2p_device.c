#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>
#include <communication/lora/lora_device/lora_p2p_device/lora_p2p_device.h>

LOG_MODULE_REGISTER(lora_p2p_device, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Configures Lora Peer-to-Peer connection and starts listening if reception is enabled
static inline int setup_lora_p2p_connection(const struct device *lora_device, bool transm_enabled);
// Starts Lora Peer-to-Peer reception
static inline int start_lora_p2p_reception(const struct device *lora_device);
// Sends Lora Peer-to-Peer package and handles errors
static inline int send_lora_p2p_package(const struct device *lora_device, uint8_t *package, int package_size);
// Verifies if transmission and reception are already correctly configured
static inline bool check_configuration(bool transm_enabled);
#ifdef CONFIG_SEND_LORAWAN
// Resets LoRa callbacks and data
static inline int reset_lora_p2p_device(const struct device *lora_device);
#endif // Device sharing

#ifdef CONFIG_RECEIVE_LORA_P2P
// Callback function to be used whenever data is received.
static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
                            int16_t rssi, int8_t snr, void *user_data);
#endif

// Variable to check if reception is enabled
static bool reception_enabled = false;
// Lora P2P transmission parameters.
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
    .tx = NULL,
};
LoraDeviceAPI lora_p2p_device = {
    .send_package = send_lora_p2p_package,
    .sync_timestamp = NULL,
    .acquire_device = setup_lora_p2p_connection,
    .release_device = NULL,
    .check_configuration = check_configuration,
};

/**
 * Definitions
 */

inline int setup_lora_p2p_connection(const struct device *lora_device, bool transm_enabled)
{
    LOG_DBG("Setting up LoRa Peer-to-Peer connection");
    int error = 0;

    if (check_configuration(transm_enabled))
    {
        LOG_DBG("LoRa Peer-to-Peer connection already configured");
        goto return_clause;
    }
#ifdef CONFIG_SEND_LORAWAN
    error = reset_lora_p2p_device(lora_device);
    if (error)
    {
        goto return_clause;
    }
#endif // Device sharing
#ifdef CONFIG_RECEIVE_LORA_P2P
    // Stop reception
    if (transm_enabled)
    {
        error = stop_lora_p2p_reception(lora_device);
        if (error)
        {
            goto return_clause;
        }
    }
#endif // CONFIG_RECEIVE_LORA_P2P
    lora_modem_config.tx = transm_enabled;
    error = lora_config(lora_device, &lora_modem_config);
    if (error)
    {
        LOG_ERR("lora_config failed: %d", error);
        goto return_clause;
    }
    LOG_DBG("LoRa Peer-to-Peer configuration successful");
#ifdef CONFIG_RECEIVE_LORA_P2P
    if (!transm_enabled)
    {
        // Start reception
        error = start_lora_p2p_reception(lora_device);
        if (error)
        {
            goto return_clause;
        }
    }
#endif // CONFIG_RECEIVE_LORA_P2P
return_clause:
    return error;
}

#ifdef CONFIG_RECEIVE_LORA_P2P
void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
                     int16_t rssi, int8_t snr, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(size);
    ARG_UNUSED(user_data);

    LOG_INF("LoRa RX RSSI: %d dBm, SNR: %d dB", rssi, snr);
    LOG_HEXDUMP_INF(data, size, "LoRa RX payload");
}
#endif // CONFIG_RECEIVE_LORA_P2P

inline int send_lora_p2p_package(const struct device *lora_device, uint8_t *package, int package_size)
{
    int error = 0;
    if (!check_configuration(true))
    {
        LOG_ERR("LoRa Peer-to-Peer connection not configured to send packages");
        return -EINVAL;
    }
    // Send using Zephyr's subsystem and check if the transmission was successful
    do
    {
        error = lora_send(lora_device, package, package_size);
    } while (error == -EAGAIN);
    if (error)
    {
        LOG_ERR("lora_send failed: %d", error);
        goto return_clause;
    }
    LOG_INF("lora_send successful");
return_clause:
    return error;
}

inline int start_lora_p2p_reception(const struct device *lora_device)
{
    int error = 0;

    if (reception_enabled)
    {
        LOG_DBG("LoRa Peer-to-Peer reception already started");
        goto return_clause;
    }
    // Start reception
    error = lora_recv_async(lora_device, lora_receive_cb, NULL);
    if (error)
    {
        LOG_ERR("Starting LoRa Peer-to-Peer reception failed: %d", error);
        goto return_clause;
    }
    reception_enabled = true;
    LOG_DBG("LoRa Peer-to-Peer reception started");
return_clause:
    return error;
}

inline int stop_lora_p2p_reception(const struct device *lora_device)
{
    int error = 0;

    if (!reception_enabled)
    {
        LOG_DBG("LoRa Peer-to-Peer reception already stopped");
        goto return_clause;
    }
    // Stop reception
    error = lora_recv_async(lora_device, NULL, NULL);
    if (error)
    {
        LOG_ERR("Stopping LoRa Peer-to-Peer reception failed: %d", error);
        goto return_clause;
    }
    reception_enabled = false;
    LOG_DBG("LoRa Peer-to-Peer reception stopped");
    // If this function is called by an external module, the transmission needs to be stopped
    // If it's called by setup, the transmission will be properly configured later
    lora_modem_config.tx = false;
return_clause:
    return error;
}

bool check_configuration(bool transm_enabled)
{
    return lora_modem_config.tx == transm_enabled &&
           reception_enabled != transm_enabled;
}

int reset_lora_p2p_device(const struct device *lora_device)
{
    int error = lora_reset(lora_device);
    if (error)
    {
        LOG_ERR("lora_reset failed: %d", error);
        goto return_clause;
    }
    reception_enabled = false;
    lora_modem_config.tx = false;
return_clause:
    return error;
}