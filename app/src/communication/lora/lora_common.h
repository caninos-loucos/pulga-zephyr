#ifndef LORA_COMMON_H
#define LORA_COMMON_H
#include <communication/comm_interface.h>

// Maximum size of a LoRa package that can be sent
#define MAX_DATA_LEN 256

// Encoding and buffering data thread
void lora_process_data(void *channel, void *buffer, void *send_thread);
// This is the function executed by the thread that actually sends the data
void lora_send_data(void *channel, void *buffer, void *);
#if defined(CONFIG_SEND_LORAWAN) && (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P))
// Prepares and checks conditions to transmit ownership of the LoRa device for the given channel
int acquire_ownership(enum ChannelType caller_channel);
#endif // Both LoRaWAN and LoRa P2P channels

#endif // LORA_COMMON_H