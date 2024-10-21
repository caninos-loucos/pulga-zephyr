#include "data_processing/text_model/text_model.h"
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(text_interface, CONFIG_APP_LOG_LEVEL);

DataAPI text_model_api;

static void text_encode(uint32_t *data_words, uint8_t *encoded_data, size_t encoded_size)
{
    snprintf(encoded_data, encoded_size, "%s\n", (char *)data_words);
}

DataAPI *register_text_model_callbacks()
{
    text_model_api.encode_verbose = text_encode;
    text_model_api.num_data_words = MAX_32_WORDS;
    return &text_model_api;
}

#ifdef CONFIG_SHELL

static int relay_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    for (int i = 1; i < argc; i++)
        insert_in_buffer((uint32_t *)argv[i], TEXT_DATA, 0);

    return 0;
}

SHELL_CMD_REGISTER(relay, NULL, "Insert one or more text items in the buffer", relay_cmd_handler);

#endif