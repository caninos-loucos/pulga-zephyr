#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <communication/lora/lora_common.h>
#include <communication/lora/lora_device/lora_device.h>

LOG_MODULE_REGISTER(lora_common, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

#if defined(CONFIG_LORA_P2P_JOIN_PACKET) || defined(CONFIG_LORAWAN_JOIN_PACKET)
struct join_variables
{
    int max_payload_size;
    uint8_t insert_index;
    int available_package_size;
    uint8_t joined_data[MAX_DATA_LEN];
};

// Resets variables used to join packets into package
void reset_join_variables(struct join_variables *join_vars, enum ChannelType channel_type);
// Adds data item from buffer to package
void add_item_to_package(struct join_variables *join_vars, uint32_t *encoded_data, uint8_t encoded_data_word_size);
#endif // Any of the two join packets configurations

/**
 * Definitions
 */

// Encoding and buffering Data thread
void lora_process_data(void *channel, void *buffer, void *send_thread)
{
    LOG_INF("Processing LoRa data started");

    int error;
    enum ChannelType channel_type = (enum ChannelType)(uintptr_t)channel;
    PulgaRingBuffer *pulga_buffer = (PulgaRingBuffer *)buffer;
    k_tid_t *send_thread_id = (k_tid_t *)send_thread;
    enum EncodingLevel encoding = MINIMALIST;

    while (1)
    {
        // Waits for data to be ready
        k_sem_take(&data_ready_sem[channel_type], K_FOREVER);
        int max_payload_size = MAX_DATA_LEN;
        if (channel_type == LORAWAN && IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET))
        {
            uint8_t unused_arg, temp_max_payload;
            // Maximum payload size determined by datarate and region
            lorawan_get_payload_sizes(&unused_arg, &temp_max_payload);
            max_payload_size = temp_max_payload;
        }
        // Encodes data item to be sent and inserts the encoded data in the internal buffer
        error = encode_and_insert(pulga_buffer, data_unit, encoding);
        if (error)
        {
            k_sem_give(&data_processed);
            continue;
        }

        if ((channel_type == LORAWAN && IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET)) ||
            (channel_type == LORA_P2P && IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET)))
        {
            // If the application is joining packets into a larger package,
            // it waits longer to wake up the sending thread, until a package with
            // maximum payload size can be assembled
            if (get_buffer_size_without_headers(pulga_buffer) < max_payload_size)
            {
                LOG_DBG("Joining more data");
                // Signals for the Communication Interface that processing is complete
                k_sem_give(&data_processed);
                continue;
            }
        }
        LOG_DBG("Waking up sending thread on channel %d", channel_type);
        k_wakeup(*send_thread_id);
        k_sem_give(&data_processed);
    }
}

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
void reset_join_variables(struct join_variables *join_vars, enum ChannelType channel_type)
{
    LOG_DBG("Resetting join variables");
    join_vars->insert_index = 0;
    // Resetting join package variables after last send
    memset(join_vars->joined_data, 0, 256);
    if (channel_type == LORAWAN)
    {
        uint8_t unused_arg, temp_max_payload;
        lorawan_get_payload_sizes(&unused_arg, &temp_max_payload);
        join_vars->max_payload_size = temp_max_payload;
    }
    else if (channel_type == LORA_P2P)
    {
        join_vars->max_payload_size = MAX_DATA_LEN;
    }
    join_vars->available_package_size = join_vars->max_payload_size;
    LOG_DBG("Maximum payload size for current datarate: %d B", join_vars->available_package_size);
}

void add_item_to_package(struct join_variables *join_vars, uint32_t *encoded_data, uint8_t encoded_data_word_size)
{
    uint8_t encoded_data_size = SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size);
    // Adds packet to package
    LOG_DBG("Adding item with size %d B to package with %d available bytes",
            encoded_data_size, join_vars->available_package_size);
    join_vars->insert_index = join_vars->max_payload_size - join_vars->available_package_size;
    bytecpy(join_vars->joined_data + join_vars->insert_index, encoded_data, encoded_data_size);
    join_vars->available_package_size -= encoded_data_size;
}
#endif // Any of the two join packets configurations

void lora_send_data(void *channel, void *buffer, void *param2)
{
    LOG_INF("Sending via lora started");
    ARG_UNUSED(param2);

    enum ChannelType channel_type = (enum ChannelType)(uintptr_t)channel;
    PulgaRingBuffer *pulga_buffer = (PulgaRingBuffer *)buffer;
#if IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET) || IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET)
    struct join_variables join_vars;

    if ((channel_type == LORAWAN && IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET)) ||
        (channel_type == LORA_P2P && IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET)))
    {
        reset_join_variables(&join_vars, channel_type);
    }
#endif // Any of the two join packets configurations

    while (1)
    {
        // After waking up, transmits until buffer is empty
        while (!buffer_is_empty(pulga_buffer))
        {
            LOG_DBG("Resetting data item variables");
            int error = 0;
            uint8_t encoded_data_word_size = MAX_32_WORDS;
            uint32_t encoded_data[MAX_32_WORDS];
            memset(encoded_data, 0, sizeof(encoded_data));

#if IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET) || IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET)
            if ((channel_type == LORAWAN && IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET)) ||
                (channel_type == LORA_P2P && IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET)))
            {
                // Peeking the size of the next item in the buffer
                error = get_item_word_size(pulga_buffer, &encoded_data_word_size);
                // Sends package as the new item wouldn't fit in it and resets package variables to form a new one
                if (join_vars.available_package_size - SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size) < 0)
                {
                    lora_device.send_package(&lora_device, channel_type, join_vars.joined_data,
                                             join_vars.max_payload_size - join_vars.available_package_size);
                    reset_join_variables(&join_vars, channel_type);
                    continue;
                }
            }
#endif // Any of the two join packets configurations

            enum DataType data_type;
            // Get the next packet from the internal buffer
            error = get_from_buffer(pulga_buffer, encoded_data, &data_type, &encoded_data_word_size);
            if (error)
            {
                continue;
            }

#if IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET) || IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET)
            if ((channel_type == LORAWAN && IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET)) ||
                (channel_type == LORA_P2P && IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET)))
            {
                add_item_to_package(&join_vars, encoded_data, encoded_data_word_size);
                continue;
            }
#endif // Any of the two join packets configurations
       // Sends the packet directly if not joining
            lora_device.send_package(&lora_device, channel_type, (uint8_t *) encoded_data,
                                     SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size));
        }
        LOG_DBG("Buffer is empty, sleeping");
        k_sleep(K_FOREVER);
    }
}