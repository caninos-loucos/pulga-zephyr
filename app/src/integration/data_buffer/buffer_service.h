#ifndef BUFFER_SERVICE_H
#define BUFFER_SERVICE_H

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <integration/data_abstraction/abstraction_service.h>

#define SIZE_BYTES_TO_32_BIT_WORDS(expr) DIV_ROUND_UP(expr, sizeof(uint32_t))
#define SIZE_32_BIT_WORDS_TO_BYTES(expr) (expr * 4)

// Maximum number of 32-bit words an item of the application buffer can have
#define MAX_32_WORDS 64

struct PulgaRingBufferPrivate;

typedef struct
{
    // Actual ring buffer
    struct ring_buf *buffer;
    // Number of data items in the buffer
    int num_data_items;
} PulgaRingBuffer;

// Declares ring buffer that will store data until it is read and sent
extern PulgaRingBuffer app_buffer;

// Initializes a ring buffer
void init_pulga_buffer(PulgaRingBuffer *pulga_buffer, struct ring_buf *ring_buffer);
// Gets item from buffer
int get_from_buffer(PulgaRingBuffer *pulga_buffer, uint32_t *data_words, enum DataType *data_type,
                    uint8_t *custom_value, uint8_t *num_words);
// Inserts data in buffer
int insert_in_buffer(PulgaRingBuffer *pulga_buffer, uint32_t *data_words, enum DataType data_type,
                     uint8_t custom_value, uint8_t num_words);
// Verifies if buffer is empty
bool buffer_is_empty(PulgaRingBuffer *pulga_buffer);
// Gets size of buffer without headers
int get_buffer_size_without_headers(PulgaRingBuffer *pulga_buffer);
// Peeks into buffer to return size of item in 32-bit words
int get_item_word_size(PulgaRingBuffer *pulga_buffer, uint8_t *item_size);

#endif /* BUFFER_SERVICE_H */