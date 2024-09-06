#ifndef UART_INTERFACE_H
#define UART_INTERFACE_H

#define UART_THREAD_STACK_SIZE 1024
#define UART_THREAD_PRIORITY 5 /* preemptible */

// Sends data via UART (terminal)
int send_data();

#endif /* UART_INTERFACE_H */