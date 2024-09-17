#include <zephyr/logging/log.h>
#include <sensors/sensors_interface.h>
#include <data_processing/data_abstraction.h>

LOG_MODULE_REGISTER(data_buffer, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// Declare and initialize ring buffer
RING_BUF_ITEM_DECLARE(data_buffer, CONFIG_BUFFER_WORDS);
// Gets type of data so appropriate handler can get item from buffer
int get_data_type();

/**
 * IMPLEMENTATIONS
 */

int get_buffer_data(uint32_t* data_model, enum DataType* data_type){
    if(!get_data_type(data_type)){
        LOG_DBG("datatype: %d\n", *data_type);
        data_apis[*data_type]->parse_buffer_data(data_model);
        return 0;
    }
    return -1;
}

int get_data_type(uint16_t* data_type){
    // Size of item type in bytes
    int type_size = 2;
    uint8_t type_bytes[type_size];
    // Peek into the ring buffer to get next item data type
    int size = ring_buf_peek(&data_buffer, type_bytes, type_size);
    if(size != type_size){
        LOG_ERR("Failed to get item type");
        return -1;
    }
    // Combines bytes into data_type
	*data_type = type_bytes[0] | (type_bytes[1] << 8);

	return 0;
}

int insert_in_buffer(enum DataType data_type, uint8_t value, 
                    uint32_t* data_words, uint8_t num_words){
    while(ring_buf_item_put(&data_buffer, data_type, value, 
                        data_words, num_words) != 0){
        LOG_ERR("Failed to insert data in ring buffer.");
        enum DataType last_item_type = -1;
        // Removes oldest item
        get_buffer_data(NULL, &last_item_type);
    }
    LOG_DBG("Wrote item to buffer starting with '0x%X' and ending with '0x%X'", 
        data_words[0], data_words[num_words - 1]);
    return 0;
}

// Verifies if buffer is empty
int data_buffer_is_empty(){
	return ring_buf_is_empty(&data_buffer);
}