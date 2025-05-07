#ifndef LORA_COMMON_H
#define LORA_COMMON_H
#include <communication/comm_interface.h>

// Maximum size of a LoRa package that can be sent
#define MAX_DATA_LEN 256

#if defined(CONFIG_LORA_P2P_JOIN_PACKET) || defined(CONFIG_LORAWAN_JOIN_PACKET)
struct join_variables
{
    int max_payload_size;
    uint8_t insert_index;
    int available_package_size;
    uint8_t joined_data[MAX_DATA_LEN];
};

// Resets variables used to join packets into package
void reset_join_variables(struct join_variables *join_vars, enum ChannelType channel_type);
// Adds data item from buffer to package
void add_item_to_package(struct join_variables *join_vars, uint32_t *encoded_data, uint8_t encoded_data_word_size);
#endif // Any of the two join packets configurations

// Encoding and buffering data thread
void lora_process_data(void *channel, void *buffer, void *send_thread);
// Prepares and checks conditions to transmit ownership of the LoRa device for the given channel
int acquire_ownership(enum ChannelType caller_channel, bool transm_enabled);
// Releases the ownership of the LoRa device for the given channel
int release_ownership(enum ChannelType caller_channel);

#endif // LORA_COMMON_H