#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <communication/lora/internal_buffer/internal_buffer.h>

LOG_MODULE_REGISTER(internal_buffer, CONFIG_APP_LOG_LEVEL);

/**
 * Definitions
 */

int encode_and_insert(struct ring_buf *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding)
{
    LOG_DBG("Encoding data item");
    int error = 0, encoded_size = 0;
    uint8_t encoded_data[256] = {0};

    // Encoding data to raw bytes
    encoded_size = encode_data(data_unit.data_words, data_unit.data_type, encoding,
                               encoded_data, sizeof(encoded_data));
    if (encoded_size < 0)
    {
        LOG_ERR("Could not encode data");
        return encoded_size;
    }
    LOG_DBG("Encoded LoRa data starting with '0x%X' and size %dB",
            encoded_data[0], encoded_size);

    // Put bytes in internal buffer, casting it to 32-bit words
    error = insert_in_buffer(buffer, (uint32_t *)encoded_data,
                             data_unit.data_type, 0, SIZE_BYTES_TO_32_BIT_WORDS(encoded_size));
    return error;
}

int get_buffer_to_package_size(struct ring_buf *buffer, int buffered_items)
{
    int internal_buffer_used_size = ring_buf_size_get(buffer);
    // Each item in the buffer has a 32-bit word header, which will be removed
    internal_buffer_used_size -= SIZE_32_BIT_WORDS_TO_BYTES(buffered_items);
    return internal_buffer_used_size;
}

int get_item_word_size(struct ring_buf *buffer, uint8_t *item_size)
{
    // Size of item header in bytes
    int header_size = 4;
    uint8_t header_bytes[header_size];
    memset(header_bytes, 0, sizeof(header_bytes));
    // Peek into the ring buffer to get next item size
    int peeked_size = ring_buf_peek(buffer, header_bytes, header_size);
    if (peeked_size != header_size)
    {
        LOG_ERR("Failed to get item size");
        return -1;
    }
    // Copies size byte into item_size
    *item_size = header_bytes[2];

    return 0;
}

bool internal_buffer_empty(struct ring_buf *buffer)
{
    return buffer_is_empty(buffer);
}

int get_internal_buffer_item(struct ring_buf *buffer, uint32_t *data_words, uint8_t *num_words)
{
    enum DataType data_type;
    return get_from_buffer(buffer, data_words, &data_type, num_words);
}
