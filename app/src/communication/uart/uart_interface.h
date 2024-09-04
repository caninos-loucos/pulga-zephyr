#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#define UART_THREAD_STACK_SIZE 4096
#define UART_THREAD_PRIORITY 5 /* preemptible */

// Time between transmission to UART
// TODO: Make it configurable from module that receives commands
extern int current_uart_interval;

// Sends data via UART (terminal)
int send_data();

#endif /* UART_INTERFACE_H */