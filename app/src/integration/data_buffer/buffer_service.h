#ifndef BUFFER_SERVICE_H
#define BUFFER_SERVICE_H

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <integration/data_abstraction/abstraction_service.h>

#define SIZE_BYTES_TO_32_BIT_WORDS(expr) DIV_ROUND_UP(expr, sizeof(uint32_t))
#define SIZE_32_BIT_WORDS_TO_BYTES(expr) (expr * 4)

// Maximum number of 32-bit words an item of the application buffer can have
#define MAX_32_WORDS 16

// Declares ring buffer that will store data until it is read and sent
extern struct ring_buf app_buffer;

// Gets item from buffer
int get_from_buffer(struct ring_buf *buffer, uint32_t *data_words, enum DataType *data_type, uint8_t *num_words);

// Inserts data in buffer
int insert_in_buffer(struct ring_buf *buffer, uint32_t *data_words, enum DataType data_type,
                     uint8_t custom_value, uint8_t num_words);

// Verifies if buffer is empty
int buffer_is_empty(struct ring_buf *buffer);

#endif /* BUFFER_SERVICE_H */