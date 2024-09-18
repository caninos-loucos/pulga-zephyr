#ifndef DATA_ABSTRACTION_H
#define DATA_ABSTRACTION_H

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

// TODO: handle multi-thread access if transmission interval
// is different for each channel
extern struct ring_buf data_buffer;
// Number of maximum 32-bit words an item of buffer can have
#define MAX_32_WORDS 16
// Offset in DataType enum. Values equal or greater than this
// mean the data came from a sensor. Other types of data will
// have a value less than this.
#define SENSOR_TYPE_OFFSET 5

// Encoding used in buffer items so program
// knows how to parse and present the data
// Sensors must be on the same order as in SensorType enum
enum DataType
{
    TEXT_DATA,
    // Sensors
    BME280_MODEL = SENSOR_TYPE_OFFSET,
    BMI160_MODEL,
    SI1133_MODEL,
    SCD30_MODEL,
    MAX_DATA_TYPE // Total number of data types
};

// Level of encoding to be used on data, depending on the communication
// channel, available memory and application
enum EncodingLevel
{
    // Splits values of data model into many individual one item sized buffers
    SPLIT_BYTES,
    // Encode each value of data model into strings that occupy less memory
    MINIMALIST,
    // Encode each value of data model into verbose strings
    VERBOSE,
};

// Functions exposed for each data type
typedef struct
{
    // Encode each value of data model into verbose strings
    void (*encode_verbose)(uint32_t *data_model, uint8_t *encoded_data, size_t encoded_size);
    // // Encode each value of data model into strings that occupy less memory
    // void* (*encode_minimalist)(uint32_t* data_model, uint8_t* encoded_data);
    // // Splits values of data model into many individual one item sized buffers
    // void* (*split_values)(uint32_t* data_model, uint8_t** value_list);
    // Size of data model in 32-bit words
    uint8_t data_model_words;
} DataAPI;

extern DataAPI* data_apis[SENSOR_TYPE_OFFSET];

// Registers callbacks for data types that don't come from sensors
int register_data_callbacks();
// Gets item from buffer
int get_buffer_data(uint32_t *data_model, enum DataType *type);
// Inserts data in buffer
int insert_in_buffer(enum DataType data_type, uint8_t value, uint32_t *data_words);
// Verifies if buffer is empty
int data_buffer_is_empty();
// Encode data to verbose presentation format
void encode_data(enum DataType data_type, enum EncodingLevel encoding,
                 uint32_t *data_model, uint8_t *encoded_data, size_t encoded_size);
// Process data type and return data API for an item
DataAPI *get_data_api(enum DataType data_type);

#endif /* DATA_ABSTRACTION_H */