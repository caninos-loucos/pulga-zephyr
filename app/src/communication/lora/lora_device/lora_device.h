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
    bool (*is_ready)(LoraDevice *lora_device);
    // Configures the LoRa device for the given channel
    int (*setup_lora_connection)(LoraDevice *lora_device, enum ChannelType caller_channel);
    // Sends a package using the LoRa device
    int (*send_package)(LoraDevice *lora_device, enum ChannelType caller_channel, uint8_t *package, uint8_t package_size);
    // Synchronizes the timestamp of the LoRa device with the network time
    // For now, this function is only used by the LoRaWAN channel
    int (*sync_timestamp)(LoraDevice *lora_device, enum ChannelType caller_channel);
};

extern LoraDevice lora_device;

#endif // LORA_DEVICE_H