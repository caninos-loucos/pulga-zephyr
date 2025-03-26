#include <zephyr/logging/log.h>
#include <integration/data_abstraction/text_model/text_model.h>
#include <integration/data_buffer/buffer_service.h>

LOG_MODULE_REGISTER(text_model, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

DataAPI text_model_api;

/**
 * IMPLEMENTATIONS
 */

// Encodes all values of payload into a string
static int text_encode(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
  return snprintf(encoded_data, encoded_size, "%s\n", (char *)data_words);
}

// Converts data words into bytes
static int encode_raw_bytes(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
  bytecpy(encoded_data, data_words, encoded_size);

  return MAX_32_WORDS;
}

DataAPI *register_text_model_callbacks()
{
  text_model_api.encode_verbose = text_encode;
  text_model_api.encode_minimalist = text_encode;
  text_model_api.encode_raw_bytes = encode_raw_bytes;
  text_model_api.num_data_words = MAX_32_WORDS;
  return &text_model_api;
}