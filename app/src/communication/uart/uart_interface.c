#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <data_processing/data_abstraction.h>
#include <communication/uart/uart_interface.h>

LOG_MODULE_REGISTER(uart_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static ChannelAPI uart_api;
static K_THREAD_STACK_DEFINE(uart_thread_stack_area, UART_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread uart_thread_data;
static k_tid_t uart_thread_id;

// Initializes and start thread to send data via UART
void init_channel();
// Functions that prints data to UART in separate thread
static void send_data_uart(void *, void *, void *);

/**
 * IMPLEMENTATIONS
 */

void init_channel()
{
    LOG_DBG("Initializing send via UART thread");
    /* Create thread and start it immediately. */
    uart_thread_id = k_thread_create(&uart_thread_data, uart_thread_stack_area,
                                     K_THREAD_STACK_SIZEOF(uart_thread_stack_area),
                                     send_data_uart, NULL, NULL, NULL,
                                     UART_THREAD_PRIORITY, 0, K_NO_WAIT);
}

static void send_data_uart(void *param0, void *param1, void *param2)
{
    LOG_DBG("Sending via UART started");
    ARG_UNUSED(param0);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    // Max fprintf character output is 4096
    uint8_t encoded_data[1024];

    while (1)
    {
        // Waits data to be ready
        k_sem_take(&data_ready_sem[UART], K_FOREVER);

        encode_data(data_unit.data_type, VERBOSE, data_unit.data_model,
                    encoded_data, sizeof(encoded_data));
        /* print data */
        printk("Encoded data: %s", encoded_data);

        // Signal back that processing is complete
        k_sem_give(&data_processed);
    }
}

ChannelAPI *register_uart_callbacks()
{
    LOG_DBG("Initializing UART callbacks");
    uart_api.init_channel = init_channel;
    return &uart_api;
}