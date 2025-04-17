#ifndef LORA_P2P_INTERFACE_H
#define LORA_P2P_INTERFACE_H

#include <communication/comm_interface.h>

#if (CONFIG_LORA_P2P_CR == 5)
#define LORA_CR CR_4_5
#elif (CONFIG_LORA_P2P_CR == 6)
#define LORA_CR CR_4_6
#elif (CONFIG_LORA_P2P_CR == 7)
#define LORA_CR CR_4_7
#elif (CONFIG_LORA_P2P_CR == 8)
#define LORA_CR CR_4_8
#else
#error "Coding Rate between 5 and 8 must be chosen"
#endif

#if (CONFIG_LORA_P2P_BW == 125)
#define LORA_BW BW_125_KHZ
#elif (CONFIG_LORA_P2P_BW == 250)
#define LORA_BW BW_250_KHZ
#elif (CONFIG_LORA_P2P_BW == 500)
#define LORA_BW BW_500_KHZ
#else
#error "Bandwidth 125, 250 or 500 must be chosen"
#endif

#if (CONFIG_LORA_P2P_SF == 12)
#define LORA_SF SF_12
#elif (CONFIG_LORA_P2P_SF == 11)
#define LORA_SF SF_11
#elif (CONFIG_LORA_P2P_SF == 10)
#define LORA_SF SF_10
#elif (CONFIG_LORA_P2P_SF == 9)
#define LORA_SF SF_9
#elif (CONFIG_LORA_P2P_SF == 8)
#define LORA_SF SF_8
#elif (CONFIG_LORA_P2P_SF == 7)
#define LORA_SF SF_7
#elif (CONFIG_LORA_P2P_SF == 6)
#define LORA_SF SF_6
#else
#error "Spreading Factor between 6 and 12 must be chosen"
#endif

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define LORA_P2P_THREAD_STACK_SIZE 2048
#define LORA_P2P_PROCESSING_PRIORITY 5 /* preemptible */

#define LORA_P2P_SEND_THREAD_STACK_SIZE 2048
#define LORA_P2P_SEND_THREAD_PRIORITY 5 /* preemptible */

// Create an internal buffer to be able to send multiple data readings in one packet
#define LORA_P2P_BUFFER_SIZE 2048

// Register lora_p2p callbacks
ChannelAPI *register_lora_p2p_callbacks();
// Configures lora_p2p connection parameters and starts listening if needed
int setup_lora_p2p_connection();

#endif /* LORA_P2P_INTERFACE_H */