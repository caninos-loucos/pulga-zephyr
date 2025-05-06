#ifndef LORA_DEVICE_H
#define LORA_DEVICE_H
#include <communication/comm_interface.h>

typedef struct PrivateAttributes LoraDevicePrivate;
typedef struct LoraDevice LoraDevice;

typedef struct LoraDeviceAPI
{
    // Sends a package using the LoRa device
    int (*send_package)(const struct device *lora_device, uint8_t *package, int package_size);
    // Synchronizes the timestamp of the LoRa device with the network time
    // For now, this function is only used by the LoRaWAN channel
    int (*sync_timestamp)(bool force_sync);
    // Setups the LoRa device for use
    int (*acquire_device)(const struct device *lora_device, bool transm_enabled);
    // Prepares the LoRa device for use by another channel
    int (*release_device)(const struct device *lora_device);
    // Verifies if device is already correctly configured
    bool (*check_configuration)(bool transm_enabled);
} LoraDeviceAPI;

struct LoraDevice
{
    // Private attributes of the LoRa device
    LoraDevicePrivate *device_private;
    // Verifies if the LoRa device is ready
    bool (*is_ready)();
    // Sends a package using the LoRa device
    int (*send_package)(enum ChannelType caller_channel, uint8_t *package, int package_size);
    // Synchronizes the timestamp of the LoRa device with the network time
    // For now, this function is only used by the LoRaWAN channel
    int (*sync_timestamp)(enum ChannelType, bool force_sync);
    // Acquires ownership and initializes LoRa device for the given channel
    int (*acquire_device)(enum ChannelType caller_channel, bool transm_enabled);
    // Safely ends the connection and releases the lock on the LoRa device
    int (*release_device)(enum ChannelType caller_channel);
};

extern LoraDevice lora_device;

#endif // LORA_DEVICE_H