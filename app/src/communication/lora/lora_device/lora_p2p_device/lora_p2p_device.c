#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>
#include <communication/lora/lora_device/lora_p2p_device/lora_p2p_device.h>

LOG_MODULE_REGISTER(lora_p2p_device, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Configures the LoRa modem for transmission and reception
static int join_p2p_network(const struct device *lora_device, bool transm_enabled);
// Gets Lora P2P transmission parameters.
// Such parameters must match between the transmitter and
// receiver for communication to occur.
static inline void lora_p2p_config(struct lora_modem_config *lora_modem_config, bool transm_enabled);
#ifdef CONFIG_RECEIVE_LORA_P2P
// Callback function to be used whenever data is received.
static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
                            int16_t rssi, int8_t snr, void *user_data);
#endif // CONFIG_RECEIVE_LORA_P2P

/**
 * Definitions
 */

int setup_lora_p2p_connection(const struct device *lora_device)
{
    LOG_DBG("Setting up LoRa Peer-to-Peer connection");
    int error = 0;

    error = join_p2p_network(lora_device,
                             (IS_ENABLED(CONFIG_SEND_LORA_P2P) &&
                              !IS_ENABLED(CONFIG_RECEIVE_LORA_P2P)));
    if (error < 0)
    {
        LOG_ERR("Failed to join LoRa Peer-to-Peer network: %d", error);
        goto return_clause;
    }

    LOG_INF("LoRa Peer-to-Peer connection setup complete");

return_clause:
    return error;
}

static int join_p2p_network(const struct device *lora_device, bool transm_enabled)
{
    int error = 0;
    struct lora_modem_config lora_modem_config;

#ifdef CONFIG_RECEIVE_LORA_P2P
    // Stop reception
    if (transm_enabled)
    {
        error = lora_recv_async(lora_device, NULL, NULL);
        if (error < 0)
        {
            LOG_ERR("Stopping LoRa Peer-to-Peer reception failed");
            goto return_clause;
        }
    }
#endif // CONFIG_RECEIVE_LORA_P2P
    lora_p2p_config(&lora_modem_config, transm_enabled);
    error = lora_config(lora_device, &lora_modem_config);
    if (error < 0)
    {
        LOG_ERR("lora_config failed");
        goto return_clause;
    }
#ifdef CONFIG_RECEIVE_LORA_P2P
    if (!transm_enabled)
    {
        // Start reception
        error = lora_recv_async(lora_device, lora_receive_cb, NULL);
        if (error < 0)
        {
            LOG_ERR("Starting LoRa Peer-to-Peer reception failed");
            goto return_clause;
        }
    }
#endif // CONFIG_RECEIVE_LORA_P2P
return_clause:
    return error;
}

void lora_p2p_config(struct lora_modem_config *lora_modem_config, bool transm_enabled)
{
    lora_modem_config->frequency = 915000000;
    lora_modem_config->bandwidth = LORA_BW;
    lora_modem_config->datarate = LORA_SF;
    lora_modem_config->preamble_len = 8;
    lora_modem_config->coding_rate = LORA_CR;
    lora_modem_config->iq_inverted = false;
    lora_modem_config->public_network = false;
    lora_modem_config->tx_power = 20;
    lora_modem_config->tx = transm_enabled;
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

int send_lora_p2p_package(const struct device *lora_device, uint8_t *package, int package_size)
{
    int error = 0;

#ifdef CONFIG_RECEIVE_LORA_P2P
    join_p2p_network(lora_device, true);
#endif // CONFIG_RECEIVE_LORA_P2P

    // Send using Zephyr's subsystem and check if the transmission was successful
    error = lora_send(lora_device, package, package_size);
    if (error < 0)
    {
        LOG_ERR("lora_send failed");
        goto return_clause;
    }
    LOG_INF("lora_send successful");

#ifdef CONFIG_RECEIVE_LORA_P2P
    join_p2p_network(lora_device, false);
#endif // CONFIG_RECEIVE_LORA_P2P

return_clause:
    return error;
}
