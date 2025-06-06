#ifndef LORA_COMMON_H
#define LORA_COMMON_H
#include <communication/comm_interface.h>

// Maximum size of a LoRa package that can be sent
#define MAX_DATA_LEN 255

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

// Variables and functions used to adapt LoRa functionality according to the protocol used
// They aim to reduce code duplication between LoraWAN, Lora P2P and future custom protocols
typedef struct LoraCommonAPI
{
    // Identifies the channel type, that is, the protocol used
    enum ChannelType channel_type;
    // Buffer used to store data until it is sent
    PulgaRingBuffer *pulga_buffer;
    // Id of the thread that sends data, which needs to be woken up
    k_tid_t send_thread_id;
    // Maximum payload size allowed for the protocol used
    uint8_t (*get_max_payload_size)();
} LoraCommonAPI;

// Encodes data and enqueues it to the given channel
int encode_and_enqueue(LoraCommonAPI *lora_api, CommunicationUnit data_unit, enum EncodingLevel encoding);
// Encoding and buffering data thread
void lora_process_data(void *channel, void *buffer, void *send_thread);
// Prepares and checks conditions to transmit ownership of the LoRa device for the given channel
int acquire_ownership(enum ChannelType caller_channel, bool transm_enabled);
// Releases the ownership of the LoRa device for the given channel
int release_ownership(enum ChannelType caller_channel);

#endif // LORA_COMMON_H