#ifndef INTERNAL_BUFFER_H
#define INTERNAL_BUFFER_H

#include <communication/comm_interface.h>

// Encodes data and inserts it into the internal buffer
int encode_and_insert(struct ring_buf *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding);
// Returns how many bytes the data currently stored in internal buffer would occupy in a package
int get_buffer_to_package_size(struct ring_buf *buffer, int buffered_items);
// Peeks into buffer to return size of item in 32-bit words
int get_item_word_size(struct ring_buf *buffer, uint8_t *item_size);
// Checks if buffer is empty
bool internal_buffer_empty(struct ring_buf *buffer);
// Gets an item from internal buffer
int get_internal_buffer_item(struct ring_buf *buffer, uint32_t *data_words, uint8_t *num_words);

#endif /* INTERNAL_BUFFER_H */