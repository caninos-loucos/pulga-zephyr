#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <communication/lora/internal_buffer/internal_buffer.h>

LOG_MODULE_REGISTER(internal_buffer, CONFIG_APP_LOG_LEVEL);

/**
 * Definitions
 */

int encode_and_insert(PulgaRingBuffer *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding)
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