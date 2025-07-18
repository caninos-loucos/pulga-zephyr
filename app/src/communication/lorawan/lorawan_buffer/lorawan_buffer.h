#ifndef LORAWAN_BUFFER_H
#define LORAWAN_BUFFER_H

#include <communication/comm_interface.h>

// Create an internal buffer to be able to send multiple data readings in one packet
#define LORAWAN_BUFFER_SIZE 2048

// Encodes data and inserts it into the internal buffer
int encode_and_insert(CommunicationUnit data_unit);
// Returns how many bytes the data currently stored in internal buffer would occupy in a package
int get_buffer_to_package_size(int buffered_items);
// Peeks into buffer to return size of item in 32-bit words
int get_item_word_size(uint8_t *item_size);
// Checks if LoRaWAN buffer is empty
bool lorawan_buffer_empty();
// Gets an item from LoRaWAN internal buffer
int get_lorawan_item(uint32_t *data_words, uint8_t *num_words);

#endif /* LORAWAN_BUFFER_H */