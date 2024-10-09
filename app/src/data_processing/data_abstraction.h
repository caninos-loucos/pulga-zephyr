#ifndef DATA_ABSTRACTION_H
#define DATA_ABSTRACTION_H

#include <zephyr/kernel.h>

// Maximum number of 32-bit words an item of buffer can have
#define MAX_32_WORDS 16
// Offset in DataType enum. Values equal or greater than this
// mean the data came from a sensor. Other types of data will
// have a value less than this.
#define SENSOR_TYPE_OFFSET 5

// Encoding used in buffer items so program
// knows how to parse and present the data
// **Sensors must be on the same order as in SensorType enum**
enum DataType
{
    TEXT_DATA,
    // Sensors
    BME280_MODEL = SENSOR_TYPE_OFFSET,
    BMI160_MODEL,
    SI1133_MODEL,
    SCD30_MODEL,
    GNSS_MODEL,
    MAX_DATA_TYPE // Total number of data types
};

// Level of encoding to be used on data, depending on the communication
// channel, available memory and application
enum EncodingLevel
{
    // Splits structured data into individual one item sized buffers
    // This translates to characteristics abstraction in Bluetooth stack
    SPLIT_DATA,
    // Encodes data into strings that occupy low memory
    MINIMALIST,
    // Encodes data into a verbose string
    VERBOSE,
};

// Functions exposed for each data type
typedef struct
{
    // Size of data of given data type in 32-bit words
    uint8_t num_data_words;
    // Encodes data into a verbose string
    int (*encode_verbose)(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size);
    // // Encodes data into strings that occupy low memory
    int (*encode_minimalist)(uint32_t* data_words, uint8_t* encoded_data, size_t encoded_size);
    // Splits structured data into individual one item sized buffers
    // void* (*split_data)(uint32_t* data_words, uint8_t** value_list);
} DataAPI;

// Registers callbacks for data types not corresponding to sensor data
int register_data_callbacks();
// Gets item from buffer
int get_from_buffer(uint32_t *data_words, enum DataType *type);
// Inserts data in buffer
int insert_in_buffer(uint32_t *data_words, enum DataType data_type, uint8_t custom_value);
// Verifies if buffer is empty
int data_buffer_is_empty();
// Encodes data to verbose presentation format
int encode_data(uint32_t *data_words, enum DataType data_type, enum EncodingLevel encoding,
                uint8_t *encoded_data, size_t encoded_size);
// Processes data type and returns correspondent data API
DataAPI *get_data_api(enum DataType data_type);

#endif /* DATA_ABSTRACTION_H */