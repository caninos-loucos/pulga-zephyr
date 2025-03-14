#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#include <data_processing/data_abstraction.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define READ_BUFFER_THREAD_STACK_SIZE 1024
#define READ_BUFFER_THREAD_PRIORITY 5 /* preemptible */

// Encoding used to map channel APIs
enum ChannelType
{
    UART,
    BLE,
    LORAWAN,
    LORAP2P,
    MAX_CHANNELS // Total number of channels
};

// API that all communication channels must implement
typedef struct
{
    // Initializes channel and starts communication
    void (*init_channel)();
} ChannelAPI;

// Data unit that will be served to communication channels
// will consist on the content and in the data type
typedef struct
{
    uint32_t data_words[MAX_32_WORDS];
    enum DataType data_type;
} CommunicationUnit;

// Semaphores to guarantee every registered channel
// will receive and process the data unit
extern struct k_sem data_ready_sem[MAX_CHANNELS];
extern struct k_sem data_processed;
extern CommunicationUnit data_unit;

// Initializes synchronization structures and
// communication for all registered channels
int init_communication();
// Registers callbacks for the used communication channels
int register_comm_callbacks();

int get_transmission_interval();
void set_transmission_interval(int interval);

#endif /* COMM_INTERFACE_H */