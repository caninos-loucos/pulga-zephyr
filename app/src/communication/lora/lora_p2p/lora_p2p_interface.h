#ifndef LORA_P2P_INTERFACE_H
#define LORA_P2P_INTERFACE_H

#include <communication/comm_interface.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define LORA_P2P_SEND_THREAD_STACK_SIZE 2048
#define LORA_P2P_SEND_THREAD_PRIORITY 5 /* preemptible */

// Create an internal buffer to be able to send multiple data readings in one packet
#define LORA_P2P_BUFFER_SIZE 2048

// Id of the thread that sends data via LoRa P2P and configures the reception
extern k_tid_t lora_p2p_send_thread_id;
// Register lora_p2p callbacks
ChannelAPI *register_lora_p2p_callbacks();

#endif /* LORA_P2P_INTERFACE_H */