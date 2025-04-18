#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <communication/lora/lora_common.h>

LOG_MODULE_REGISTER(lora_common, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

/**
 * Definitions
 */

// Get the devicetree node for the LoRa hardware.
// It should have an alias declared as lora0 to be properly found!
#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS(DEFAULT_RADIO_NODE, okay),
             "No default LoRa radio specified in DT");

K_SEM_DEFINE(lora_sem, 1, 1);
PulgaLoraDevice lora_device = {
    .device = DEVICE_DT_GET(DT_ALIAS(lora0)),
    .device_sem = &lora_sem,
};

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

#if defined(CONFIG_LORA_P2P_JOIN_PACKET) || defined(CONFIG_LORAWAN_JOIN_PACKET)
void reset_join_variables(int *max_payload_size, uint8_t *insert_index,
                          int *available_package_size, uint8_t *joined_data, enum ChannelType channel_type)
{
    LOG_DBG("Resetting join variables");
    *insert_index = 0;
    // Resetting join package variables after last send
    memset(joined_data, 0, 256);
    if (channel_type == LORAWAN)
    {
        uint8_t unused_arg, temp_max_payload;
        lorawan_get_payload_sizes(&unused_arg, &temp_max_payload);
        *max_payload_size = temp_max_payload;
    }
    else if (channel_type == LORA_P2P)
    {
        *max_payload_size = MAX_DATA_LEN;
    }
    *available_package_size = *max_payload_size;
    LOG_DBG("Maximum payload size for current datarate: %d B", *available_package_size);
}

void add_item_to_package(uint8_t encoded_data_word_size, int max_payload_size,
                         int *available_package_size, uint8_t *joined_data,
                         uint8_t *insert_index, uint32_t *encoded_data)
{
    uint8_t encoded_data_size = SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size);
    // Adds packet to package
    LOG_DBG("Adding item with size %d B to package with %d available bytes",
            encoded_data_size, *available_package_size);
    *insert_index = max_payload_size - *available_package_size;
    bytecpy(joined_data + *insert_index, encoded_data, encoded_data_size);
    *available_package_size -= encoded_data_size;
}
#endif // Any of the two join packets configurations