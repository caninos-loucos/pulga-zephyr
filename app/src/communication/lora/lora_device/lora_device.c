#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <communication/lora/lora_device/lora_device.h>
#include <communication/lora/lora_device/lorawan_device/lorawan_device.h>
#include <communication/lora/lora_device/lora_p2p_device/lora_p2p_device.h>

LOG_MODULE_REGISTER(lora_device, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

typedef enum
{
    OWNERSHIP_LORA_P2P = LORA_P2P,
    OWNERSHIP_LORAWAN = LORAWAN,
} DeviceOwnership;

struct PrivateAttributes
{
    // Channel which currently owns the LoRa device
    DeviceOwnership ownership;
    // LoRa device used for communication by LoRaWAN and LoRa P2P channels
    const struct device *device;
    // This semaphore is used to prevent multiple threads from accessing the LoRa device at the same time
    struct k_sem *device_sem;
};

// Verifies if the LoRa device is ready
static inline bool is_ready(LoraDevice *lora_device);
// Configures the LoRa device for the given channel
static int setup_lora_connection(LoraDevice *lora_device, enum ChannelType caller_channel);
// Sends a package using the LoRa device
static int send_package(LoraDevice *lora_device, enum ChannelType caller_channel, uint8_t *package, uint8_t package_size);
// Acquires ownership of the LoRa device for the given channel
static int acquire_ownership(LoraDevice *lora_device, enum ChannelType caller_channel);
// Verifies if the LoRa device is owned by the channel calling the function
static inline int check_ownership(LoraDevice *lora_device, enum ChannelType caller_channel);
// Synchronizes the timestamp of the LoRa device with the network time
// For now, this function is only used by the LoRaWAN channel
static int sync_timestamp(LoraDevice *lora_device, enum ChannelType caller_channel);

/**
 * Definitions
 */

// Get the devicetree node for the LoRa hardware.
// It should have an alias declared as lora0 to be properly found!
#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
             "No default LoRa radio specified in DT");

K_SEM_DEFINE(lora_sem, 1, 1);

static struct PrivateAttributes device_private =
    {
        .device = DEVICE_DT_GET(DT_ALIAS(lora0)),
        .device_sem = &lora_sem,
#ifdef CONFIG_SEND_LORAWAN
        .ownership = OWNERSHIP_LORAWAN,
#else
        .ownership = OWNERSHIP_LORA_P2P,
#endif
};

LoraDevice lora_device =
    {
        .device_private = &device_private,
        .is_ready = is_ready,
        .setup_lora_connection = setup_lora_connection,
        .send_package = send_package,
        .sync_timestamp = sync_timestamp,
};

int check_ownership(LoraDevice *lora_device, enum ChannelType caller_channel)
{
    // Check if the LoRa device is owned by the channel calling the function
    if ((enum ChannelType)lora_device->device_private->ownership != caller_channel)
    {
        LOG_ERR("LoRa device is not owned by channel %d", caller_channel);
        return -EPERM;
    }
    return 0;
}

int acquire_ownership(LoraDevice *lora_device, enum ChannelType caller_channel)
{
    int error = 0;

    k_sem_take(lora_device->device_private->device_sem, K_FOREVER);
    if ((enum ChannelType)lora_device->device_private->ownership == caller_channel)
    {
        LOG_ERR("LoRa device is already owned by channel %d", caller_channel);
        error = -EBUSY;
    }
    lora_device->device_private->ownership = caller_channel;
    k_sem_give(lora_device->device_private->device_sem);
    return error;
}

bool is_ready(LoraDevice *lora_device)
{
    int ready = 0;

    k_sem_take(lora_device->device_private->device_sem, K_FOREVER);
    ready = device_is_ready(lora_device->device_private->device);
    if (!ready)
    {
        LOG_ERR("%s: device not ready.", lora_device->device_private->device->name);
    }
    k_sem_give(lora_device->device_private->device_sem);
    return ready;
}

int setup_lora_connection(LoraDevice *lora_device, enum ChannelType caller_channel)
{
    int error = 0;
    k_sem_take(lora_device->device_private->device_sem, K_FOREVER);
    error = check_ownership(lora_device, caller_channel);
    if (error)
    {
        goto return_clause;
    }
#ifdef CONFIG_SEND_LORAWAN
    if (caller_channel == LORAWAN)
    {
        error = setup_lorawan_connection(lora_device);
        if (error)
        {
            LOG_ERR("Failed to setup LoRaWAN connection: %d", error);
        }
        goto return_clause;
    }
#endif
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
    if (caller_channel == LORA_P2P)
    {
        error = setup_lora_p2p_connection(lora_device->device_private->device);
        if (error)
        {
            LOG_ERR("Failed to setup LoRa Peer-to-Peer connection: %d", error);
        }
        goto return_clause;
    }
#endif
return_clause:
    k_sem_give(lora_device->device_private->device_sem);
    return 0;
}

int send_package(LoraDevice *lora_device, enum ChannelType caller_channel, uint8_t *package, uint8_t package_size)
{
    int error = 0;
    k_sem_take(lora_device->device_private->device_sem, K_FOREVER);
    error = check_ownership(lora_device, caller_channel);
    if (error)
    {
        goto return_clause;
    }
#ifdef CONFIG_SEND_LORAWAN
    if (caller_channel == LORAWAN)
    {
        error = send_lorawan_package(lora_device, package, package_size);
        if (error)
        {
            LOG_ERR("Failed to send LoRaWAN package: %d", error);
        }
        goto return_clause;
    }
#endif
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
    if (caller_channel == LORA_P2P)
    {
        error = send_lora_p2p_package(lora_device->device_private->device, package, package_size);
        if (error)
        {
            LOG_ERR("Failed to send LoRa Peer-to-Peer package: %d", error);
        }
        goto return_clause;
    }
#endif
return_clause:
    k_sem_give(lora_device->device_private->device_sem);
    return 0;
}

int sync_timestamp(LoraDevice *lora_device, enum ChannelType caller_channel)
{
    int error = 0;
    k_sem_take(lora_device->device_private->device_sem, K_FOREVER);
    error = check_ownership(lora_device, caller_channel);
    if (error)
    {
        goto return_clause;
    }
#ifdef CONFIG_SEND_LORAWAN
    if (caller_channel == LORAWAN)
    {
        error = get_network_time(false);
        if (error)
        {
            LOG_ERR("Failed to sync timestamp: %d", error);
        }
        goto return_clause;
    }
#endif
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
    if (caller_channel == LORA_P2P)
    {
        LOG_ERR("Not implemented.");
        goto return_clause;
    }
#endif
return_clause:
    k_sem_give(lora_device->device_private->device_sem);
    return 0;
}
