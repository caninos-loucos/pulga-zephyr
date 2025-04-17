#ifndef LORA_COMMON_H
#define LORA_COMMON_H

#include <zephyr/kernel.h>
#include <communication/comm_interface.h>
#include <zephyr/device.h>

typedef struct
{
    // LoRa device used for communication by LoRaWAN and LoRa P2P channels
    const struct device *device;
    // This semaphore is used to prevent multiple threads from accessing the LoRa device at the same time
    struct k_sem *device_sem;
} PulgaLoraDevice;

extern PulgaLoraDevice lora_device;

// Encodes data and inserts it into the internal buffer
int encode_and_insert(PulgaRingBuffer *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding);

#endif /* LORA_COMMON_H */