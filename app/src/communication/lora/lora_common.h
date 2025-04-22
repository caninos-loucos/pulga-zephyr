#ifndef LORA_COMMON_H
#define LORA_COMMON_H
#include <communication/comm_interface.h>

// Maximum size of a LoRa package that can be sent
#define MAX_DATA_LEN 256

// Encoding and buffering data thread
void lora_process_data(void *channel, void *buffer, void *send_thread);
// Encodes data and inserts it into the internal buffer
int encode_and_insert(PulgaRingBuffer *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding);
// This is the function executed by the thread that actually sends the data
void lora_send_data(void *channel, void *buffer, void *);

#endif // LORA_COMMON_H