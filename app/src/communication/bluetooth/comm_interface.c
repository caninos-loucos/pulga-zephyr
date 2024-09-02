#include "comm_ble_peripheral.c"

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define BLE_THREAD_STACK_SIZE 1024
#define BLE_THREAD_PRIORITY 5 /* preemptible */

static K_THREAD_STACK_DEFINE(ble_thread_stack_area, BLE_THREAD_STACK_SIZE);

typedef enum{ 
    BLE, // Bluetooth Low Energy
    LORA, // LoraWan 
    P2P // Lora P2P 
} comm_channel_enum;

static int send_data(char* message, comm_channel_enum used_channel){
    struct k_thread ble_thread_data;
	k_tid_t ble_thread_id;

	/* Create thread and start it immediately. */
	ble_thread_id = k_thread_create(&ble_thread_data, ble_thread_stack_area,
							 K_THREAD_STACK_SIZEOF(ble_thread_stack_area),
							 init_peripheral, (void *)CONFIG_SAMPLE_CONN_ITERATIONS,
                             NULL, NULL, BLE_THREAD_PRIORITY, 0, K_NO_WAIT);
    return 0;
}

static inline int distribute_data(char* message, comm_channel_enum* used_channels, \
    short int length_channels){
    short int i_channels;

    for(i_channels = 0; i_channels < length_channels; i_channels++){
        switch(used_channels[i_channels]){
            case BLE:
            send_data(message, BLE);
            break;
            case P2P:
            break;
            case LORA:
            break;

            default:
            break;
        }
    }
    return 0;
}