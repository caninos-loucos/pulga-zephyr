#ifndef LORA_COMMON_H
#define LORA_COMMON_H

#include <zephyr/kernel.h>
#include <communication/comm_interface.h>
#include <zephyr/device.h>

// Maximum size of a LoRa package that can be sent
#define MAX_DATA_LEN 256

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

#if defined(CONFIG_LORA_P2P_JOIN_PACKET) || defined(CONFIG_LORAWAN_JOIN_PACKET)
// Resets variables used to join packets into package
void reset_join_variables(int *max_payload_size, uint8_t *insert_index,
                          int *available_package_size, uint8_t *joined_data, enum ChannelType channel_type);
// Adds data item from buffer to package
void add_item_to_package(uint8_t encoded_data_word_size, int max_payload_size,
                                int *available_package_size, uint8_t *joined_data,
                                uint8_t *insert_index, uint32_t *encoded_data);
#endif // Any of the two join packets configurations

#endif // LORA_COMMON_H