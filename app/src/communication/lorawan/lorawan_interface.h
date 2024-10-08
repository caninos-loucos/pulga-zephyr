#ifndef LORAWAN_INTERFACE_H
#define LORAWAN_INTERFACE_H

#include <communication/comm_interface.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define LORAWAN_THREAD_STACK_SIZE 8192
#define LORAWAN_THREAD_PRIORITY 5 /* preemptible */

// Create an internal buffer to be able to send multiple data readings in one packet
#define LORAWAN_BUFFER_SIZE 2048
#define LORAWAN_WORKQUEUE_THREAD_STACK_SIZE 2048

// Register lorawan callbacks
ChannelAPI *register_lorawan_callbacks();

#endif /* LORAWAN_INTERFACE_H */