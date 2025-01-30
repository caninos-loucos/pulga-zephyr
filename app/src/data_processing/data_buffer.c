#include <zephyr/logging/log.h>
#include <data_processing/data_abstraction.h>

LOG_MODULE_REGISTER(data_buffer, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// Initializes ring buffer that will store data until it is read and sent
RING_BUF_ITEM_DECLARE(app_buffer, CONFIG_BUFFER_WORDS);
// Peeks into buffer to return type of data
static int get_data_type(struct ring_buf *buffer, enum DataType *data_type);
// Parses data from buffer according to data type
static int parse_buffer_data(struct ring_buf *buffer, uint32_t *data_words, enum DataType data_type);

/**
 * IMPLEMENTATIONS
 */

int get_from_buffer(struct ring_buf *buffer, uint32_t *data_words, enum DataType *data_type)
{
    if (get_data_type(buffer, data_type) == 0 &&
        parse_buffer_data(buffer, data_words, *data_type) == 0)
    {
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

int insert_in_buffer(struct ring_buf *buffer, uint32_t *data_words, enum DataType data_type, uint8_t custom_value)
{
    // Number of 32-bit words in data_words
    uint8_t num_words = get_data_api(data_type)->num_data_words;

    // Removes oldest items from buffer until new item fits
    while (ring_buf_item_put(buffer, data_type, custom_value,
                             data_words, num_words) != 0)
    {
        LOG_ERR("Failed to insert data in ring buffer.");
        enum DataType last_item_type = -1;
        // Discards item
        get_from_buffer(buffer, NULL, &last_item_type);
    }
    LOG_DBG("Wrote item to buffer starting with '0x%X' and ending with '0x%X'",
            data_words[0], data_words[num_words - 1]);
    return 0;
}

// Verifies if buffer is empty
int buffer_is_empty(struct ring_buf *buffer)
{
    return ring_buf_is_empty(buffer);
}

int parse_buffer_data(struct ring_buf *buffer, uint32_t *data_words, enum DataType data_type)
{
    // Number of 32-bit words in data_words
    uint8_t custom_value = 0, num_words = get_data_api(data_type)->num_data_words;
    uint16_t type;
    int error = 0;

    // Discarding oldest data
    if (data_words == NULL)
    {
        LOG_DBG("Discarding data item");
    }
    // Tries to get actual item from buffer
    error = ring_buf_item_get(buffer, &type, &custom_value, data_words, &num_words);
    if (error)
    {
        LOG_ERR("Failed to get data from ring buffer.");
        data_words = NULL;
        return -1;
    }
    LOG_DBG("Got item from buffer with datatype %d, "
            "starting with '0x%X' and ending with '0x%X'",
            data_type, data_words[0], data_words[num_words - 1]);
    return 0;
}