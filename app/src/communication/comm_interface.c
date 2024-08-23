#include "comm_ble_peripheral.c"

typedef enum{ 
    BLE, // Bluetooth Low Energy
    LORA, // LoraWan 
    P2P // Lora P2P 
} comm_channel_enum;

static int send_data(char* message, comm_channel_enum used_channel){
    init_peripheral(CONFIG_SAMPLE_CONN_ITERATIONS);
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