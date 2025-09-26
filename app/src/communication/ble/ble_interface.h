#ifndef BLE_INTERFACE_H
#define BLE_INTERFACE_H

#include <communication/comm_interface.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define BLE_THREAD_STACK_SIZE 5120
#define BLE_THREAD_PRIORITY 5 /* preemptible */

// Register BLE callbacks
ChannelAPI *register_ble_callbacks();

#endif /* BLE_INTERFACE_H */