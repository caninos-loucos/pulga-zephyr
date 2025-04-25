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
    OWNERSHIP_LORAWAN = LORAWAN,
    OWNERSHIP_LORA_P2P = LORA_P2P,
};

typedef struct DeviceOwnership
{
    enum DeviceOwner channel;
    // Protects the ownership variable
    struct k_sem *ownership_sem;
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
static inline bool is_ready();
// Configures the LoRa device for the given channel
static int setup_lora_connection(enum ChannelType caller_channel, bool transm_enabled);
// Sends a package using the LoRa device
static int send_package(enum ChannelType caller_channel, uint8_t *package, int package_size);
// Synchronizes the timestamp of the LoRa device with the network time
// For now, this function is only used by the LoRaWAN channel
static int sync_timestamp(enum ChannelType caller_channel, bool force_sync);
// Verifies if the LoRa device is owned by the channel calling the function
static bool check_ownership(enum ChannelType caller_channel);
// Acquires ownership of the LoRa device for the given channel
static int acquire_device(enum ChannelType caller_channel);
// Releases the lock on the LoRa device
static int release_device(enum ChannelType caller_channel);

/**
 * Definitions
 */

// Get the devicetree node for the LoRa hardware.
// It should have an alias declared as lora0 to be properly found!
#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
             "No default LoRa radio specified in DT");

K_SEM_DEFINE(lora_sem, 1, 1);
K_SEM_DEFINE(ownership_sem, 1, 1);

static struct PrivateAttributes device_private = {
    .device = DEVICE_DT_GET(DT_ALIAS(lora0)),
    .device_sem = &lora_sem,
    .ownership =
        {
#if (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)) && defined(CONFIG_SEND_LORAWAN)
            .channel = OWNERSHIP_FREE,
#elif (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)) && !defined(CONFIG_SEND_LORAWAN)
            .channel = OWNERSHIP_LORA_P2P,
#elif !(defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)) && defined(CONFIG_SEND_LORAWAN)
            .channel = OWNERSHIP_LORAWAN,
#endif // CONFIG_SEND_LORAWAN
            .ownership_sem = &ownership_sem,
        },
};

LoraDevice lora_device = {
    .device_private = &device_private,
    .is_ready = is_ready,
    .setup_lora_connection = setup_lora_connection,
    .send_package = send_package,
    .sync_timestamp = sync_timestamp,
    .check_ownership = check_ownership,
    .acquire_device = acquire_device,
    .release_device = release_device,
};

bool is_ready()
{
    int ready = 0;

    k_sem_take(lora_device.device_private->device_sem, K_FOREVER);
    ready = device_is_ready(lora_device.device_private->device);
    if (!ready)
    {
        LOG_ERR("%s: device not ready.", lora_device.device_private->device->name);
    }
    k_sem_give(lora_device.device_private->device_sem);
    return ready;
}

int setup_lora_connection(enum ChannelType caller_channel, bool transm_enabled)
{
    int error = 0;
    if (!check_ownership(caller_channel))
    {
        enum DeviceOwner owner = lora_device.device_private->ownership.channel;
        LOG_DBG("CHANNEL %d - Tried to setup connection but device is owned by %d", caller_channel, owner);
        return -EPERM;
    }
    k_sem_take(lora_device.device_private->device_sem, K_FOREVER);
#ifdef CONFIG_SEND_LORAWAN
    if (caller_channel == LORAWAN)
    {
        error = setup_lorawan_connection();
        goto return_clause;
    }
#endif
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
    if (caller_channel == LORA_P2P)
    {
        error = setup_lora_p2p_connection(lora_device.device_private->device, transm_enabled);
        goto return_clause;
    }
#endif
return_clause:
    k_sem_give(lora_device.device_private->device_sem);
    return error;
}

int send_package(enum ChannelType caller_channel, uint8_t *package, int package_size)
{
    int error = 0;
    if (!check_ownership(caller_channel))
    {
        enum DeviceOwner owner = lora_device.device_private->ownership.channel;
        LOG_DBG("CHANNEL %d - Tried to send package but device is owned by %d", caller_channel, owner);
        return -EPERM;
    }
    k_sem_take(lora_device.device_private->device_sem, K_FOREVER);
#ifdef CONFIG_SEND_LORAWAN
    if (caller_channel == LORAWAN)
    {
        error = send_lorawan_package(package, (uint8_t)package_size);
        goto return_clause;
    }
#endif
#if defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P)
    if (caller_channel == LORA_P2P)
    {
        error = send_lora_p2p_package(lora_device.device_private->device, package, package_size);
        goto return_clause;
    }
#endif
return_clause:
    k_sem_give(lora_device.device_private->device_sem);
    return error;
}

int sync_timestamp(enum ChannelType caller_channel, bool force_sync)
{
    int error = 0;
    if (!check_ownership(caller_channel))
    {
        enum DeviceOwner owner = lora_device.device_private->ownership.channel;
        LOG_DBG("CHANNEL %d - Tried to synchronize but device is owned by %d", caller_channel, owner);
        return -EPERM;
    }
    k_sem_take(lora_device.device_private->device_sem, K_FOREVER);
#ifdef CONFIG_SEND_LORAWAN
    if (caller_channel == LORAWAN)
    {
        error = get_network_time(force_sync);
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
    k_sem_give(lora_device.device_private->device_sem);
    return error;
}

bool check_ownership(enum ChannelType caller_channel)
{
    k_sem_take(lora_device.device_private->device_sem, K_FOREVER);
    // Check if the LoRa device is owned by the channel calling the function
    bool owned = (enum ChannelType)lora_device.device_private->ownership.channel == caller_channel;
    k_sem_give(lora_device.device_private->device_sem);
    return owned;
}

int acquire_device(enum ChannelType caller_channel)
{
    if (check_ownership(caller_channel))
    {
        // The LoRa device is already owned by the channel calling the function
        LOG_DBG("CHANNEL %d - Tried to acquire LoRa device already owned by this channel", caller_channel);
        return -EBUSY;
    }
    // Waits for the LoRa device to be released
    k_sem_take(lora_device.device_private->ownership.ownership_sem, K_FOREVER);
    k_sem_take(lora_device.device_private->device_sem, K_FOREVER);
    lora_device.device_private->ownership.channel = caller_channel;
    k_sem_give(lora_device.device_private->device_sem);
    LOG_DBG("CHANNEL %d - LoRa device acquired", caller_channel);
    return 0;
}

int release_device(enum ChannelType caller_channel)
{
    if (!check_ownership(caller_channel))
    {
        enum DeviceOwner owner = lora_device.device_private->ownership.channel;
        if (owner == OWNERSHIP_FREE)
        {
            LOG_DBG("CHANNEL %d - LoRa device is already free", caller_channel);
            return 0;
        }
        LOG_DBG("CHANNEL %d - Tried to release device owned by %d", caller_channel, owner);
        return -EPERM;
    }
    k_sem_take(lora_device.device_private->device_sem, K_FOREVER);
    lora_device.device_private->ownership.channel = OWNERSHIP_FREE;
    k_sem_give(lora_device.device_private->ownership.ownership_sem);
    k_sem_give(lora_device.device_private->device_sem);
    LOG_DBG("CHANNEL %d - LoRa device released", caller_channel);
    return 0;
}