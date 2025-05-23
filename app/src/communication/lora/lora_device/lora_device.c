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

// Represents the channel which is currently using the LoRa device
enum DeviceOwner
{
    OWNERSHIP_FREE = -1,
    OWNERSHIP_LORA_P2P,
    OWNERSHIP_LORAWAN,
    MAX_OWNERSHIP,
    OWNERSHIP_INVALID = 0xFF,
};

typedef struct DeviceOwnership
{
    enum DeviceOwner channel;
    // Protects the ownership variable
    struct k_sem *ownership_sem[MAX_OWNERSHIP];
} DeviceOwnership;

struct PrivateAttributes
{
    // Channel which currently owns the LoRa device
    DeviceOwnership ownership;
    // LoRa device used for communication by LoRaWAN and LoRa P2P channels
    const struct device *device;
    // This semaphore is used to prevent multiple threads from accessing the LoRa device at the same time
    struct k_sem *device_sem;
    // APIs for different channels using the LoRa device
    LoraDeviceAPI *device_apis[MAX_OWNERSHIP];
};

#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
K_SEM_DEFINE(lora_p2p_sem, 1, 1);
#endif
#ifdef CONFIG_SEND_LORAWAN
K_SEM_DEFINE(lorawan_sem, 1, 1);
#endif

// Get the devicetree node for the LoRa hardware.
// It should have an alias declared as lora0 to be properly found!
#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
             "No default LoRa radio specified in DT");

K_SEM_DEFINE(lora_sem, 1, 1);

static struct PrivateAttributes device_private = {
    .device = DEVICE_DT_GET(DEFAULT_RADIO_NODE),
    .device_sem = &lora_sem,
    .device_apis = {
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
        [OWNERSHIP_LORA_P2P] = &lora_p2p_device,
#endif
#if defined(CONFIG_SEND_LORAWAN)
        [OWNERSHIP_LORAWAN] = &lorawan_device,
#endif
    },
    .ownership = {.channel = OWNERSHIP_FREE, .ownership_sem = {
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
                                                 [OWNERSHIP_LORA_P2P] = &lora_p2p_sem,
#endif
#if defined(CONFIG_SEND_LORAWAN)
                                                 [OWNERSHIP_LORAWAN] = &lorawan_sem,
#endif
                                             }}};

// Returns the device owner for the given channel type
static inline enum DeviceOwner get_owner(enum ChannelType channel_type);
// Verifies if the LoRa device is owned by the channel calling the function
static inline bool check_ownership(enum DeviceOwner caller_owner);
// Verifies if device is already correctly configured
static inline bool check_configuration(enum DeviceOwner caller_owner, bool transm_enabled);
static inline bool is_ready();
static inline int send_package(enum ChannelType caller_channel, uint8_t *package, int package_size);
static inline int sync_timestamp(enum ChannelType caller_channel, bool force_sync);
static int acquire_device(enum ChannelType caller_channel, bool transm_enabled);
static int release_device(enum ChannelType caller_channel);

LoraDevice lora_device = {
    .device_private = &device_private,
    .is_ready = is_ready,
    .send_package = send_package,
    .sync_timestamp = sync_timestamp,
    .acquire_device = acquire_device,
    .release_device = release_device,
};

/**
 * Definitions
 */

enum DeviceOwner get_owner(enum ChannelType channel_type)
{
    switch (channel_type)
    {
    case LORA_P2P:
        return OWNERSHIP_LORA_P2P;
    case LORAWAN:
        return OWNERSHIP_LORAWAN;
    default:
        LOG_ERR("Invalid channel type: %d", channel_type);
        return OWNERSHIP_INVALID;
    }
    return OWNERSHIP_INVALID;
}

bool is_ready()
{
    int ready = 0;

    k_sem_take(device_private.device_sem, K_FOREVER);
    ready = device_is_ready(device_private.device);
    if (!ready)
    {
        LOG_ERR("%s: device not ready.", device_private.device->name);
    }
    k_sem_give(device_private.device_sem);
    return ready;
}

bool check_ownership(enum DeviceOwner caller_owner)
{
    k_sem_take(device_private.device_sem, K_FOREVER);
    // Check if the LoRa device is owned by the channel calling the function
    bool owned = device_private.ownership.channel == caller_owner;
    k_sem_give(device_private.device_sem);
    return owned;
}

bool check_configuration(enum DeviceOwner caller_owner, bool transm_enabled)
{
    k_sem_take(device_private.device_sem, K_FOREVER);
    bool configured = true;
    if (device_private.device_apis[caller_owner]->check_configuration)
    {
        configured = device_private.device_apis[caller_owner]->check_configuration(transm_enabled);
    }
    k_sem_give(device_private.device_sem);
    return configured;
}

int send_package(enum ChannelType caller_channel, uint8_t *package, int package_size)
{
    int error = 0;
    enum DeviceOwner caller_owner = get_owner(caller_channel);
    if (!check_ownership(caller_owner))
    {
        enum DeviceOwner current_owner = device_private.ownership.channel;
        LOG_DBG("CHANNEL %d - Tried to send package but device is owned by %d",
                caller_channel, current_owner);
        return -EPERM;
    }
    k_sem_take(device_private.device_sem, K_FOREVER);
    error = device_private.device_apis[caller_owner]->send_package(device_private.device,
                                                                   package, package_size);
    k_sem_give(device_private.device_sem);
    return error;
}

int sync_timestamp(enum ChannelType caller_channel, bool force_sync)
{
    int error = 0;
    enum DeviceOwner caller_owner = get_owner(caller_channel);
    if (!check_ownership(caller_owner))
    {
        enum DeviceOwner current_owner = device_private.ownership.channel;
        LOG_DBG("CHANNEL %d - Tried to synchronize but device is owned by %d", caller_channel, current_owner);
        return -EPERM;
    }
    if (!device_private.device_apis[caller_owner]->sync_timestamp)
    {
        LOG_ERR("CHANNEL %d - Timestamp synchronization not supported", caller_channel);
        k_sem_give(device_private.device_sem);
        return -ENOTSUP;
    }
    k_sem_take(device_private.device_sem, K_FOREVER);
    error = device_private.device_apis[caller_owner]->sync_timestamp(force_sync);
    k_sem_give(device_private.device_sem);
    return error;
}

int acquire_device(enum ChannelType caller_channel, bool transm_enabled)
{
    LOG_DBG("CHANNEL %d - Acquiring LoRa device", caller_channel);
    int error = 0;
    enum DeviceOwner caller_owner = get_owner(caller_channel);
    if (check_ownership(caller_owner))
    {
        if (check_configuration(caller_owner, transm_enabled))
        {
            // The LoRa device is already owned by the channel calling the function
            LOG_DBG("CHANNEL %d - Tried to acquire LoRa device already "
                    "owned and configured by this channel",
                    caller_channel);
            return -EBUSY;
        }
    }
// Checks if both LoRaWAN and LoRa P2P protocols are enabled.
#if defined(CONFIG_SEND_LORAWAN) && \
    (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P))
    else if (device_private.ownership.channel != OWNERSHIP_FREE)
    {
        // Waits for the LoRa device to be released
        k_sem_take(device_private.ownership.ownership_sem[caller_owner], K_FOREVER);
    }
#endif // Device sharing

    k_sem_take(device_private.device_sem, K_FOREVER);

// Checks if both LoRaWAN and LoRa P2P protocols are enabled.
#if defined(CONFIG_SEND_LORAWAN) && \
    (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P))
    if (caller_owner == OWNERSHIP_LORAWAN && IS_ENABLED(CONFIG_RECEIVE_LORA_P2P))
    {
        // Stops Lora Peer-to-Peer reception before initializing LoRaWAN
        error = stop_lora_p2p_reception(device_private.device);
        if (error < 0)
        {
            goto return_clause;
        }
    }
#endif // Device sharing

    error = device_private.device_apis[caller_owner]->acquire_device(device_private.device,
                                                                     transm_enabled);
    if (error)
    {
        goto return_clause;
    }
    device_private.ownership.channel = caller_owner;
    LOG_DBG("CHANNEL %d - LoRa device acquired", caller_channel);
return_clause:
    if (error)
    {
        // If the device cannot be acquired, release the ownership
        LOG_ERR("CHANNEL %d - Failed to acquire LoRa device: %d", caller_channel, error);
// Checks if both LoRaWAN and LoRa P2P protocols are enabled.
#if defined(CONFIG_SEND_LORAWAN) && \
    (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P))
        if (!check_ownership(caller_owner))
        {
            k_sem_give(device_private.ownership.ownership_sem[caller_owner]);
        }
#endif
    }
    k_sem_give(device_private.device_sem);
    return error;
}

int release_device(enum ChannelType caller_channel)
{
    LOG_DBG("CHANNEL %d - Releasing LoRa device", caller_channel);
    int error = 0;
    enum DeviceOwner caller_owner = get_owner(caller_channel);
    enum DeviceOwner current_owner = device_private.ownership.channel;
    if (current_owner == OWNERSHIP_FREE)
    {
        LOG_ERR("CHANNEL %d - LoRa device is already free", caller_channel);
        return -EINVAL;
    }
    if (!check_ownership(caller_owner))
    {
        LOG_DBG("CHANNEL %d - Tried to release device owned by %d", caller_channel, current_owner);
        return -EPERM;
    }

    k_sem_take(device_private.device_sem, K_FOREVER);
    if (device_private.device_apis[caller_owner]->release_device)
    {
        error = device_private.device_apis[caller_owner]->release_device(device_private.device);
        if (error)
        {
            LOG_ERR("CHANNEL %d - Failed to release LoRa device: %d", caller_channel, error);
            goto return_clause;
        }
    }
    device_private.ownership.channel = OWNERSHIP_FREE;

// Checks if both LoRaWAN and LoRa P2P protocols are enabled.
#if defined(CONFIG_SEND_LORAWAN) && \
    (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P))
    if (caller_owner == OWNERSHIP_LORA_P2P)
    {
        // If the channel is LoRa P2P, it releases the semaphore for the LoRaWAN channel
        k_sem_give(device_private.ownership.ownership_sem[OWNERSHIP_LORAWAN]);
    }
    else
    {
        // If the channel is LoRaWAN, it releases the semaphore for the LoRa P2P channel
        k_sem_give(device_private.ownership.ownership_sem[OWNERSHIP_LORA_P2P]);
    }
#endif // Device sharing

    LOG_DBG("CHANNEL %d - LoRa device released", caller_channel);
return_clause:
    k_sem_give(device_private.device_sem);
    return error;
}