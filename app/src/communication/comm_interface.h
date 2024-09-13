#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#include <data_processing/data_abstraction.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define READ_BUFFER_THREAD_STACK_SIZE 1024
#define READ_BUFFER_THREAD_PRIORITY 5 /* preemptible */

// Encoding used to map channels APIs
enum ChannelType {
    UART,
    BLE,
    LORAWAN,
    LORAP2P,
	MAX_CHANNELS // Total number of channels
};

typedef struct{
	void (*init_channel)();
} ChannelAPI;

typedef struct{
    uint32_t data_model[MAX_32_WORDS];
    enum DataType data_type;
} CommunicationUnit;

// Semaphores to guarantee every registered channel
// will process the data
extern struct k_sem data_ready_sem[MAX_CHANNELS];
extern struct k_sem data_processed;
extern CommunicationUnit data_unit;

int init_communication();
// Registers callbacks for the used communication channels
int register_comm_callbacks();

#endif /* COMM_INTERFACE_H */