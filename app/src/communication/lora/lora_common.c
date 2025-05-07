#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <communication/lora/lora_common.h>
#include <communication/lora/lora_device/lora_device.h>
#include <communication/lora/lorawan/lorawan_interface.h>
#include <communication/lora/lora_p2p/lora_p2p_interface.h>

LOG_MODULE_REGISTER(lora_common, CONFIG_APP_LOG_LEVEL);

/**
 * Declarations
 */

// Gets the maximum payload size for the given channel type
static inline void get_max_payload_size(enum ChannelType channel_type, int *max_payload_size);
// Encodes data and inserts it into the internal buffer
static inline int encode_and_insert(PulgaRingBuffer *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding);

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
        int max_payload_size;
        get_max_payload_size(channel_type, &max_payload_size);

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

void get_max_payload_size(enum ChannelType channel_type, int *max_payload_size)
{
    *max_payload_size = MAX_DATA_LEN;
#if defined(CONFIG_LORAWAN_JOIN_PACKET)
    if (channel_type == LORAWAN && IS_ENABLED(CONFIG_LORAWAN_JOIN_PACKET))
    {
        uint8_t unused_arg, temp_max_payload;
        // Maximum payload size determined by datarate and region
        lorawan_get_payload_sizes(&unused_arg, &temp_max_payload);
        *max_payload_size = temp_max_payload;
    }
#endif
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
    error = insert_in_buffer(buffer, (uint32_t *)encoded_data, data_unit.data_type,
                             0, SIZE_BYTES_TO_32_BIT_WORDS(encoded_size));
    return error;
}

#if defined(CONFIG_LORA_P2P_JOIN_PACKET) || defined(CONFIG_LORAWAN_JOIN_PACKET)
void reset_join_variables(struct join_variables *join_vars, enum ChannelType channel_type)
{
    LOG_DBG("CHANNEL %d - Resetting join variables", channel_type);
    join_vars->insert_index = 0;
    // Resetting join package variables after last send
    memset(join_vars->joined_data, 0, MAX_DATA_LEN);
    join_vars->max_payload_size = MAX_DATA_LEN;
#ifdef CONFIG_LORAWAN_JOIN_PACKET
    if (channel_type == LORAWAN)
    {
        uint8_t unused_arg, temp_max_payload;
        lorawan_get_payload_sizes(&unused_arg, &temp_max_payload);
        join_vars->max_payload_size = temp_max_payload;
    }
#endif // CONFIG_LORAWAN_JOIN_PACKET
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

inline int acquire_ownership(enum ChannelType caller_channel, bool transm_enabled)
{
    int error = 0;
    error = lora_device.acquire_device(caller_channel, transm_enabled);
    if (!error || error == -EBUSY)
    {
        return 0;
    }
    return error;
}

inline int release_ownership(enum ChannelType caller_channel)
{
    int error = 0;
    error = lora_device.release_device(caller_channel);
    if (!error || error == -EINVAL)
    {
#ifdef CONFIG_RECEIVE_LORA_P2P
        // Wakes P2P up to configure reception again
        if (caller_channel == LORAWAN && lora_p2p_send_thread_id)
        {
            k_wakeup(lora_p2p_send_thread_id);
        }
#endif
        return 0;
    }
    return error;
}