#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H
#include <communication/comm_interface.h>

#define UART_THREAD_STACK_SIZE 5120
#define UART_THREAD_PRIORITY 5 /* preemptible */

// Register UART callbacks
ChannelAPI* register_uart_callbacks();

#endif /* UART_INTERFACE_H */