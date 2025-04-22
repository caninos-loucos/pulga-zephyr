#ifndef LORAWAN_DEVICE_H
#define LORAWAN_DEVICE_H
#include <communication/lora/lora_device/lora_device.h>

// Configures lorawan connection, joining the network
int setup_lorawan_connection(LoraDevice *lora_device);
// Sends LoRaWAN package and handles errors
int send_lorawan_package(LoraDevice *lora_device, uint8_t *package, uint8_t package_size);
// // Acquires ownership of the LoRa device, changing the device configuration
// int switch_ownership(LoraDevice lora_device);
#ifdef CONFIG_EVENT_TIMESTAMP_LORAWAN
// Sends a request to the network to get the current time and
// sets it as the synchronization time
int get_network_time(bool force_request);
#endif // CONFIG_EVENT_TIMESTAMP_LORAWAN

#endif // LORAWAN_DEVICE_H