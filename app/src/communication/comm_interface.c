#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <communication/comm_interface.h>
#include <communication/uart/uart_interface.h>

LOG_MODULE_REGISTER(comm_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

struct k_sem data_ready_sem[MAX_CHANNELS];
struct k_sem data_processed;
CommunicationUnit data_unit;
// List of registered communication APIs
static ChannelAPI *channel_apis[MAX_CHANNELS] = {0};
// Stack of reading buffer thread
static K_THREAD_STACK_DEFINE(read_buffer_thread_stack_area, READ_BUFFER_THREAD_STACK_SIZE);
// Thread control block - metadata
static struct k_thread read_buffer_thread_data;
static k_tid_t read_buffer_thread_id;
// Time between transmissions
// #TODO: Make it configurable from module that receives commands
// #TODO: Make it configurable per communication channel
static int current_transmission_interval = CONFIG_TRANSMISSION_INTERVAL;

// Initializes all registered channels and synchronization structures
static int init_channels();
// Starts communication work - getting from buffer and waking up channels
static void start_communication();
// Retrieves data from buffer and handles synchronization
static void read_and_notify(void *, void *, void *);

/**
 * IMPLEMENTATIONS
 */

int register_comm_callbacks()
{
    LOG_DBG("Registering communication callbacks");

#if defined(CONFIG_SEND_UART)
    channel_apis[UART] = register_uart_callbacks();
#endif /* CONFIG_SEND_UART */

    return 0;
}

int init_communication()
{
    if (init_channels())
    {
        LOG_ERR("Error initializing channels");
        return -1;
    }
    start_communication();
    return 0;
}

int init_channels()
{
    LOG_DBG("Initializing communication channels");
    // Counts registered channels so reading buffer thread can know when
    // all of them have already processed the data
    int registered_comm_channels = 0, error = 0;
    // Calls init channel function for each registered API
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        if (channel_apis[i] != NULL)
        {
            LOG_DBG("Initializing channel %d", i);
            error = k_sem_init(&data_ready_sem[i], 0, 1);
            if (error)
            {
                LOG_ERR("Failed to initialize data ready semaphore: %d", error);
                return error;
            }
            channel_apis[i]->init_channel();
            registered_comm_channels++;
        }
    }
    error = k_sem_init(&data_processed, 0, registered_comm_channels);
    if (error)
    {
        LOG_ERR("Failed to initialize data processed semaphore: %d", error);
        return error;
    }

    return 0;
}

void start_communication()
{
    int error = 0;
    // Creates thread and starts it immediately.
    read_buffer_thread_id = k_thread_create(&read_buffer_thread_data, read_buffer_thread_stack_area,
                                            K_THREAD_STACK_SIZEOF(read_buffer_thread_stack_area),
                                            read_and_notify, NULL, NULL, NULL,
                                            READ_BUFFER_THREAD_PRIORITY, 0, K_NO_WAIT);
    error = k_thread_name_set(read_buffer_thread_id, "read_buffer");
    if (error)
    {
        LOG_ERR("Failed to set read buffer thread name: %d", error);
    }
}

static void read_and_notify(void *param0, void *param1, void *param2)
{
    LOG_INF("Reading buffer thread started");
    ARG_UNUSED(param0);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);

    while (1)
    {
        // Waits for specified time
        k_sleep(K_MSEC(current_transmission_interval));
        // After waking up, transmits until buffer is empty
        while (data_buffer_is_empty() == false)
        {
            if (get_from_buffer(data_unit.data_words, &data_unit.data_type) == 0)
            {
                // Notifies each registered channel that a new data unit is ready
                for (int i = 0; i < MAX_CHANNELS; i++)
                {
                    if (channel_apis[i] != NULL)
                    {
                        k_sem_give(&data_ready_sem[i]);
                    }
                }

                // Waits until all registered channels have processed the data unit
                for (int i = 0; i < MAX_CHANNELS; i++)
                {
                    if (channel_apis[i] != NULL)
                    {
                        k_sem_take(&data_processed, K_FOREVER);
                    }
                }
            }
        }
    }
}

void set_transmission_interval(int interval)
{
    current_transmission_interval = interval;
}

int get_transmission_interval()
{
    return current_transmission_interval;
}

#ifdef CONFIG_SHELL

static int set_transmission_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    if (argc > 1)
        set_transmission_interval((int)strtol(argv[1], NULL, 10));

    return 0;
}

static int get_transmission_interval_cmd_handler(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Transmission interval is %d milliseconds", get_transmission_interval());

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(transmission_interval_subcmds,
                               SHELL_CMD(set, NULL, "Set transmission interval", set_transmission_interval_cmd_handler),
                               SHELL_CMD(get, NULL, "Get transmission interval", get_transmission_interval_cmd_handler),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(transmission_interval, &transmission_interval_subcmds, "Get or set communication interface's transmission interval", NULL);

#endif