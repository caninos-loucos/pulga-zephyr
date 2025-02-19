#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <communication/lorawan/lorawan_buffer/lorawan_buffer.h>

LOG_MODULE_REGISTER(lorawan_buffer, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Defines the internal buffer for used to store data while waiting a previous package to be sent
RING_BUF_ITEM_DECLARE(lorawan_internal_buffer, LORAWAN_BUFFER_SIZE);

/**
 * Definitions
 */

int encode_and_insert(CommunicationUnit data_unit)
{
    LOG_DBG("Encoding data item");
    int error = 0, encoded_size = 0;
    uint8_t encoded_data[256] = {0};

    // Encoding data to raw bytes
    encoded_size = encode_data(data_unit.data_words, data_unit.data_type, MINIMALIST,
                               encoded_data, sizeof(encoded_data));
    if (encoded_size < 0)
    {
        LOG_ERR("Could not encode data");
        return encoded_size;
    }
    LOG_DBG("Encoded LoRa data starting with '0x%X' and size %dB",
            encoded_data[0], encoded_size);

    // Put bytes in internal buffer, casting it to 32-bit words
    error = insert_in_buffer(&lorawan_internal_buffer, (uint32_t *)encoded_data,
                             data_unit.data_type, 0, SIZE_BYTES_TO_32_BIT_WORDS(encoded_size));
    return error;
}

// Returns how many bytes the data currently stored in internal buffer would occupy in a package
int get_buffer_to_package_size(int buffered_items)
{
	int internal_buffer_used_size = ring_buf_size_get(&lorawan_internal_buffer);
	// Each item in the buffer has a 32-bit word header, which will be removed
	internal_buffer_used_size -= SIZE_32_BIT_WORDS_TO_BYTES(buffered_items);
	return internal_buffer_used_size;
}

int get_item_word_size(uint8_t *item_size)
{
	// Size of item header in bytes
	int header_size = 4;
	uint8_t header_bytes[header_size];
	memset(header_bytes, 0, sizeof(header_bytes));
	// Peek into the ring buffer to get next item size
	int peeked_size = ring_buf_peek(&lorawan_internal_buffer, header_bytes, header_size);
	if (peeked_size != header_size)
	{
		LOG_ERR("Failed to get item size");
		return -1;
	}
	// Copies size byte into item_size
	*item_size = header_bytes[2];

	return 0;
}

bool lorawan_buffer_empty()
{
    return buffer_is_empty(&lorawan_internal_buffer);
}

int get_lorawan_item(uint32_t *data_words, uint8_t *num_words)
{
    enum DataType data_type;
    return get_from_buffer(&lorawan_internal_buffer, data_words, &data_type, num_words);
}
