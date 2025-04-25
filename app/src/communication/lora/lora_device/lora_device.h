#ifndef LORA_DEVICE_H
#define LORA_DEVICE_H
#include <communication/comm_interface.h>

typedef struct PrivateAttributes LoraDevicePrivate;
typedef struct LoraDevice LoraDevice;

struct LoraDevice
{
    // Private attributes of the LoRa device
    LoraDevicePrivate *device_private;
    // Verifies if the LoRa device is ready
    bool (*is_ready)();
    // Configures the LoRa device for the given channel
    int (*setup_lora_connection)(enum ChannelType caller_channel, bool transm_enabled);
    // Sends a package using the LoRa device
    int (*send_package)(enum ChannelType caller_channel, uint8_t *package, int package_size);
    // Synchronizes the timestamp of the LoRa device with the network time
    // For now, this function is only used by the LoRaWAN channel
    int (*sync_timestamp)(enum ChannelType, bool force_sync);
    // Verifies if the LoRa device is owned by the channel calling the function
    bool (*check_ownership)(enum ChannelType caller_channel);
    // Acquires ownership of the LoRa device for the given channel
    int (*acquire_device)(enum ChannelType caller_channel);
    // Releases the lock on the LoRa device
    int (*release_device)(enum ChannelType caller_channel);
};

extern LoraDevice lora_device;

#endif // LORA_DEVICE_H