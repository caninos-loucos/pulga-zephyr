#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#include <communication/comm_interface.h>

// Register UART callbacks
ChannelAPI *register_uart_callbacks();

#endif /* UART_INTERFACE_H */