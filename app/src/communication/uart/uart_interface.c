#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <communication/uart/uart_interface.h>

LOG_MODULE_REGISTER(uart_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// Implementation of ChannelAPI for UART channel
static ChannelAPI uart_api;
// Function that prints data to configured UART (boards/caninos/pulga/pulga-pinctrl.dtsi)
static void uart_send_data(CommunicationUnit *data_unit);

/**
 * IMPLEMENTATIONS
 */

ChannelAPI *register_uart_callbacks()
{
    LOG_DBG("Initializing UART callbacks");
    uart_api.init_channel = NULL;
    uart_api.enqueue_data = uart_send_data;
    return &uart_api;
}

static void uart_send_data(CommunicationUnit *data_unit)
{

    uint8_t encoded_data[1024];
    int size;
    // Encoding data to verbose string
    size = encode_data(data_unit->data_words, data_unit->data_type, VERBOSE,
                       encoded_data, sizeof(encoded_data));
    if (size >= 0)
        printk("%s\n", encoded_data);
    else
        LOG_ERR("Could not encode data");

    // Signals back that UART sending is complete
    k_sem_give(&data_processed);
}