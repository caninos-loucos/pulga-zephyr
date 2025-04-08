#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <communication/uart/uart_interface.h>

LOG_MODULE_REGISTER(uart_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// Implementation of ChannelAPI for UART channel
static ChannelAPI uart_api;
// Stack of UART communication thread
static K_THREAD_STACK_DEFINE(uart_thread_stack_area, UART_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread uart_thread_data;
static k_tid_t uart_thread_id;

// Initializes and starts thread to send data via UART
static int uart_init_channel();
// Functions that prints data to UART in separate thread
static void uart_send_data(void *, void *, void *);

/**
 * IMPLEMENTATIONS
 */

static int uart_init_channel()
{
    LOG_DBG("Initializing send via UART thread");
    int ret = 0;
    // Create thread and starts it immediately
    uart_thread_id = k_thread_create(&uart_thread_data, uart_thread_stack_area,
                                     K_THREAD_STACK_SIZEOF(uart_thread_stack_area),
                                     uart_send_data, NULL, NULL, NULL,
                                     UART_THREAD_PRIORITY, 0, K_NO_WAIT);
    ret = k_thread_name_set(uart_thread_id, "send_uart");
    if (ret)
    {
        LOG_ERR("Failed to set read buffer thread name: %d", ret);
    }
    return ret;
}

static void uart_send_data(void *param0, void *param1, void *param2)
{
    LOG_DBG("Sending via UART started");
    ARG_UNUSED(param0);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    // Max fprintf character output is 4096
    uint8_t encoded_data[1024];
    int size;

    while (1)
    {
        // Waits data to be ready
        k_sem_take(&data_ready_sem[UART], K_FOREVER);

        // Encoding data to verbose string
        size = encode_data(data_unit.data_words, data_unit.data_type, VERBOSE,
                           encoded_data, sizeof(encoded_data));
        if (size >= 0)
            printk("%s\n", encoded_data);
        else
            LOG_ERR("Could not encode data");

        // Signals back that UART sending is complete
        k_sem_give(&data_processed);
    }
}

ChannelAPI *register_uart_callbacks()
{
    LOG_DBG("Initializing UART callbacks");
    uart_api.init_channel = uart_init_channel;
    return &uart_api;
}