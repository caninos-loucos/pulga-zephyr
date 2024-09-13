#ifndef DATA_ABSTRACTION_H
#define DATA_ABSTRACTION_H

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

// TODO: handle multi-thread access if transmission interval
// is different for each channel
extern struct ring_buf data_buffer;
// Number of 32-bit words the buffers can hold
// Using 224 out of 256kB of memory
#define BUFFER_ITEMS 57344
// Number of maximum 32-bit words an item of buffer can have
#define MAX_32_WORDS 16

// Encoding used in buffer items so program
// knows how to parse and present the data
enum DataType {
    BME280_MODEL,
    BMI160_MODEL,
    SI1133_MODEL,
    SCD30_MODEL,
    TEXT_DATA,
	MAX_DATA_TYPE // Total number of data types
};

// Functions exposed for each data type
typedef struct{
    // Correctly parses data from buffer for each data type
	void (*parse_buffer_data)(uint32_t* data_model);
    // Encode each value of data model into verbose strings
	void (*encode_verbose)(uint32_t* data_model, uint8_t* encoded_data, size_t encoded_size);
    // // Encode each value of data model into strings that occupy less memory
	// void* (*encode_minimalist)(uint32_t* data_model, uint8_t* encoded_data);
    // // Splits values of data model into many individual one item sized buffers
	// void* (*split_values)(uint32_t* data_model, uint8_t** value_list);
} DataAPI;

extern DataAPI* data_apis[MAX_DATA_TYPE];

// Registers data callbacks for the used sensors
int register_data_callbacks();
// Gets item from buffer
int get_buffer_data(uint32_t* data_model, enum DataType* type);
// Inserts data in buffer
int insert_in_buffer(enum DataType type, uint8_t value, uint32_t* data_words, uint8_t num_words);
// Verifies if buffer is empty
int data_buffer_is_empty();

#endif /* DATA_ABSTRACTION_H */