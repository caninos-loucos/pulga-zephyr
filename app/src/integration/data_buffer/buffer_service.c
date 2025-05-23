#include <zephyr/logging/log.h>
#include <integration/data_buffer/buffer_service.h>

LOG_MODULE_REGISTER(data_buffer, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// Initializes ring buffer that will store data until it is read and sent
RING_BUF_ITEM_DECLARE(app_ring_buffer, CONFIG_BUFFER_WORDS);
PulgaRingBuffer app_buffer = {
    .buffer = &app_ring_buffer,
    .num_data_items = 0,
};
// Peeks into buffer to return type of data
static int get_data_type(struct ring_buf *buffer, enum DataType *data_type);
// Parses data from buffer according to data type
static int parse_buffer_data(struct ring_buf *buffer, uint32_t *data_words, enum DataType data_type,
                             uint8_t *custom_value, uint8_t *num_words);

/**
 * IMPLEMENTATIONS
 */

int get_from_buffer(PulgaRingBuffer *pulga_buffer, uint32_t *data_words, enum DataType *data_type,
                    uint8_t *custom_value, uint8_t *num_words)
{
    if (get_data_type(pulga_buffer->buffer, data_type) == 0 &&
        parse_buffer_data(pulga_buffer->buffer, data_words, *data_type, custom_value, num_words) == 0)
    {
        pulga_buffer->num_data_items--;
        return 0;
    }
    return -1;
}

int get_data_type(struct ring_buf *buffer, enum DataType *data_type)
{
    // Size of item type in bytes
    int type_size = 2;
    uint8_t type_bytes[type_size];
    // Peek into the ring buffer to get next item data type
    int size = ring_buf_peek(buffer, type_bytes, type_size);
    if (size != type_size)
    {
        LOG_ERR("Failed to get item type");
        return -1;
    }
    // Combines bytes into data_type
    memcpy(data_type, type_bytes, type_size);

    return 0;
}

int insert_in_buffer(PulgaRingBuffer *pulga_buffer, uint32_t *data_words, enum DataType data_type,
                     uint8_t custom_value, uint8_t num_words)
{
    // Removes oldest items from buffer until new item fits
    while (ring_buf_item_put(pulga_buffer->buffer, data_type, custom_value,
                             data_words, num_words) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
        enum DataType last_item_type = -1;
        uint8_t last_custom_value = 0;
        // Discards item
        get_from_buffer(pulga_buffer, NULL, &last_item_type, &last_custom_value, NULL);
    }
    pulga_buffer->num_data_items++;
    LOG_DBG("Wrote item to buffer starting with '0x%X' and ending with '0x%X'",
            data_words[0], data_words[num_words - 1]);
    return 0;
}

// Verifies if buffer is empty
bool buffer_is_empty(PulgaRingBuffer *pulga_buffer)
{
    return ring_buf_is_empty(pulga_buffer->buffer);
}

int parse_buffer_data(struct ring_buf *buffer, uint32_t *data_words, enum DataType data_type,
                      uint8_t *custom_value, uint8_t *num_words)
{
    // Number of 32-bit words in data_words
    uint8_t *data_size, data_size_value = 0;
    uint16_t type;
    int error = 0;

    data_size = &data_size_value;
    // num_words is NULL when the caller function doesn't know the size of data_words
    if (num_words == NULL)
    {
        *data_size = get_data_api(data_type)->num_data_words;
    }
    else
    {
        data_size = num_words;
    }

    // Discarding oldest data
    if (data_words == NULL)
    {
        LOG_DBG("Discarding data item");
    }
    // Tries to get actual item from buffer
    error = ring_buf_item_get(buffer, &type, custom_value, data_words, data_size);
    if (error)
    {
        LOG_ERR("Failed to get data from ring buffer.");
        data_words = NULL;
        return -1;
    }
    LOG_DBG("Got item from buffer with datatype %d, "
            "starting with '0x%X' and ending with '0x%X'",
            data_type, data_words[0], data_words[*data_size - 1]);
    return 0;
}

int get_buffer_size_without_headers(PulgaRingBuffer *pulga_buffer)
{
    int used_size = ring_buf_size_get(pulga_buffer->buffer);
    // Each item in the buffer has a 32-bit word header, which will be removed
    used_size -= SIZE_32_BIT_WORDS_TO_BYTES(pulga_buffer->num_data_items);
    return used_size;
}

void init_pulga_buffer(PulgaRingBuffer *pulga_buffer, struct ring_buf *ring_buffer)
{
    // Initializes ring buffer
    pulga_buffer->buffer = ring_buffer;
    pulga_buffer->num_data_items = 0;
    LOG_DBG("Initialized ring buffer");
}

int get_item_word_size(PulgaRingBuffer *pulga_buffer, uint8_t *item_size)
{
    // Size of item header in bytes
    int header_size = 4;
    uint8_t header_bytes[header_size];
    memset(header_bytes, 0, sizeof(header_bytes));
    // Peek into the ring buffer to get next item size
    int peeked_size = ring_buf_peek(pulga_buffer->buffer, header_bytes, header_size);
    if (peeked_size != header_size)
    {
        LOG_ERR("Failed to get item size");
        return -1;
    }
    // Copies size byte into item_size
    *item_size = header_bytes[2];

    return 0;
}