
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <communication/comm_interface.h>
#include <communication/uart/uart_interface.h>

LOG_MODULE_REGISTER(comm_interface, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

static ChannelAPI* channels_apis[MAX_CHANNELS] = {0};
struct k_sem data_ready_sem[MAX_CHANNELS];
struct k_sem data_processed;
CommunicationUnit data_unit;

static K_THREAD_STACK_DEFINE(read_buffer_thread_stack_area, READ_BUFFER_THREAD_STACK_SIZE);
//Thread control block - metadata
static struct k_thread read_buffer_thread_data;
static k_tid_t read_buffer_thread_id;
// Time between transmissions
// TODO: Make it configurable from module that receives commands
static int current_transmission_interval = CONFIG_TRANSMISSION_INTERVAL;

// Initializes all channels
static int init_channels();
// Starts communication work - getting from buffer and mapping to channels
static int start_communication();
// Retrieval from buffer and mapping handler per type of data
// and communication channel in separate thread
static void read_and_send(void *, void *, void *);

/**
 * IMPLEMENTATIONS
 */

int register_comm_callbacks(){
	LOG_DBG("Registering communication callbacks");

	#if defined(CONFIG_SEND_UART)
	channels_apis[UART] = register_uart_callbacks();
	#endif /* CONFIG_SEND_UART */

	return 0;
}

int init_communication(){
    if(init_channels()){
		LOG_ERR("Error initializing channels");
		return -1;
	}
	if(start_communication()){
		LOG_ERR("Error starting communication");
		return -2;
	}
	return 0;
}

int init_channels(){ 
    int used_comm_channels = 0;
	// Calls init channel function for each registered API
	LOG_DBG("Initializing communication channels");
	for(int i = 0; i < MAX_CHANNELS; i++){
        if(channels_apis[i] != NULL){
            LOG_DBG("Initializing channel %d", i);
            k_sem_init(&data_ready_sem[i], 0, 1);
		    channels_apis[i]->init_channel();
            used_comm_channels++;
        }
	}
    k_sem_init(&data_processed, 0, used_comm_channels);

    return 0;
}

int start_communication(){
	/* Create thread and start it immediately. */
	read_buffer_thread_id = k_thread_create(&read_buffer_thread_data, read_buffer_thread_stack_area,
							 K_THREAD_STACK_SIZEOF(read_buffer_thread_stack_area),
                             read_and_send, NULL, NULL, NULL, 
                             READ_BUFFER_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(read_buffer_thread_id, "read_buffer");
    return 0;
}

static void read_and_send(void *param0, void *param1, void *param2){
    LOG_INF("Reading buffer thread started");
    ARG_UNUSED(param0);
    ARG_UNUSED(param1);
	ARG_UNUSED(param2);

    while(1){
        k_sleep(K_MSEC(current_transmission_interval));
        while(!data_buffer_is_empty()){
            if (!get_buffer_data(data_unit.data_model, &data_unit.data_type)) {
                // Signal each registered channel that new data is ready
                for (int i = 0; i < MAX_CHANNELS; i++) {
                    if (channels_apis[i] != NULL) {
                        k_sem_give(&data_ready_sem[i]); 
                    }
                }

                // Wait until all registered channels have processed the data
                for (int i = 0; i < MAX_CHANNELS; i++) {
                    if (channels_apis[i] != NULL) {
                        k_sem_take(&data_processed, K_FOREVER);
                    }
                }
            }
        }
    }
}
