#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <communication/lora/lora_common.h>
#include <communication/lora/lora_device/lora_device.h>
#include <communication/lora/lorawan/lorawan_interface.h>

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

// Encodes data and inserts it into the internal buffer
static int encode_and_insert(PulgaRingBuffer *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding);
// Acquires LoRa device if necessary and sends the package
static int send_package(LoraDevice *lora_device, enum ChannelType caller_channel, uint8_t *package, uint8_t package_size);

/**
 * Definitions
 */

// Encoding and buffering Data thread
void lora_process_data(void *channel, void *buffer, void *send_thread)
{
    int error;
    enum ChannelType channel_type = (enum ChannelType)(uintptr_t)channel;
    PulgaRingBuffer *pulga_buffer = (PulgaRingBuffer *)buffer;
    k_tid_t *send_thread_id = (k_tid_t *)send_thread;
    enum EncodingLevel encoding = MINIMALIST;

    LOG_INF("CHANNEL %d - Processing LoRa data started", channel_type);
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
                LOG_DBG("CHANNEL %d - Joining more data", channel_type);
                // Signals for the Communication Interface that processing is complete
                k_sem_give(&data_processed);
                continue;
            }
        }
        LOG_DBG("CHANNEL %d - Waking up sending thread", channel_type);
        k_wakeup(*send_thread_id);
        k_sem_give(&data_processed);
    }
}

int encode_and_insert(PulgaRingBuffer *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding)
{
    LOG_DBG("Encoding data item");
    int error = 0, encoded_size = 0;
    uint8_t encoded_data[MAX_DATA_LEN] = {0};

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
    LOG_DBG("CHANNEL %d - Resetting join variables", channel_type);
    join_vars->insert_index = 0;
    // Resetting join package variables after last send
    memset(join_vars->joined_data, 0, MAX_DATA_LEN);
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
    LOG_DBG("CHANNEL %d - Maximum payload size for current datarate: %d B",
            channel_type, join_vars->available_package_size);
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
    ARG_UNUSED(param2);

    enum ChannelType channel_type = (enum ChannelType)(uintptr_t)channel;
    PulgaRingBuffer *pulga_buffer = (PulgaRingBuffer *)buffer;
#if IS_ENABLED(CONFIG_LORA_P2P_JOIN_PACKET) || IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET)
    struct join_variables join_vars;

    LOG_INF("CHANNEL %d - Sending via lora started", channel_type);

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
            LOG_DBG("CHANNEL %d - Resetting data item variables", channel_type);
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
                    send_package(&lora_device, channel_type, join_vars.joined_data,
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
            send_package(&lora_device, channel_type, (uint8_t *)encoded_data,
                         SIZE_32_BIT_WORDS_TO_BYTES(encoded_data_word_size));
        }
#if defined(CONFIG_SEND_LORAWAN) && defined(CONFIG_RECEIVE_LORA_P2P)
        // If LoRaWAN is used, it releases the ownership of the LoRa device
        if (channel_type == LORAWAN && IS_ENABLED(CONFIG_RECEIVE_LORA_P2P))
        {
            // Activates P2P so it can receive data
            lora_device.release_ownership(&lora_device, LORAWAN);
            acquire_ownership(LORA_P2P);
        }
#endif // Both LoRaWAN and LoRa P2P channels
        LOG_DBG("CHANNEL %d - Buffer is empty, sleeping", channel_type);
        k_sleep(K_FOREVER);
    }
}

int send_package(LoraDevice *lora_device, enum ChannelType caller_channel, uint8_t *package, uint8_t package_size)
{
    int error = 0;
#if defined(CONFIG_SEND_LORAWAN) && (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P))
    error = acquire_ownership(caller_channel);
    if (error)
    {
        return error;
    }
#endif // Both LoRaWAN and LoRa P2P channels
    error = lora_device->send_package(lora_device, caller_channel, package, package_size);
    return error;
}

#if defined(CONFIG_SEND_LORAWAN) && (defined(CONFIG_SEND_LORA_P2P) || defined(CONFIG_RECEIVE_LORA_P2P))
int acquire_ownership(enum ChannelType caller_channel)
{
    int error = 0;
    // Checks if the channel is already using the LoRa device
    error = lora_device.check_ownership(&lora_device, caller_channel);
    if (!error)
    {
        LOG_DBG("CHANNEL %d - LoRa device already owned by this channel", caller_channel);
        goto return_clause;
    }

    // if (caller_channel == LORA_P2P)
    // {
    //     if (!is_lorawan_buffer_empty())
    //     {
    //         LOG_ERR("CHANNEL %d - LoRaWAN has priority and its buffer is not empty, cannot acquire ownership",
    //                 caller_channel);
    //         error = -EBUSY;
    //         goto return_clause;
    //     }
    //     lora_device.release_ownership(&lora_device, LORAWAN);
    // }
    // LoRaWAN has priority over LoRa P2P
    else if (caller_channel == LORAWAN)
    {
        lora_device.release_ownership(&lora_device, LORA_P2P);
    }
    // Acquires ownership of the LoRa device
    error = lora_device.acquire_ownership(&lora_device, caller_channel);
    if (error)
    {
        goto return_clause;
    }
    error = lora_device.setup_lora_connection(&lora_device, caller_channel);
    if (error)
    {
        goto return_clause;
    }
    LOG_DBG("CHANNEL %d - LoRa device acquired", caller_channel);
return_clause:
    return error;
}
#endif // Both LoRaWAN and LoRa P2P channels