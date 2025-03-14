#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <data_processing/text_model/text_model.h>

LOG_MODULE_REGISTER(text_model, CONFIG_APP_LOG_LEVEL);

DataAPI text_model_api;

static int text_encode(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
  return snprintf(encoded_data, encoded_size, "%s\n", (char *)data_words);
}

DataAPI *register_text_model_callbacks()
{
  text_model_api.encode_verbose = text_encode;
  text_model_api.encode_minimalist = text_encode;
  text_model_api.num_data_words = MAX_32_WORDS;
  return &text_model_api;
}

#ifdef CONFIG_SHELL

static int forward_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
  if (argc != 2)
  {
    shell_print(sh, "Must provide one payload");
    return -1;
  }

  char payload[SIZE_32_BIT_WORDS_TO_BYTES((MAX_32_WORDS - 1))] = {0};
  uint32_t data_words[MAX_32_WORDS] = {0};

  snprintf(payload, sizeof(payload), "%s", argv[1]);
  memcpy(&data_words, payload, sizeof(payload));

  if (insert_in_buffer(&app_buffer, data_words, TEXT_DATA, 0, MAX_32_WORDS) != 0)
  {
    LOG_ERR("Failed to insert data in ring buffer.");
    return -1;
  }

  return 0;
}

SHELL_CMD_REGISTER(forward, NULL, "Insert one or more text items in the buffer", forward_cmd_handler);

#endif