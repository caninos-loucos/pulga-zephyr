#include <zephyr/logging/log.h>
#include <sensors/sensors_interface.h>
#include <data_processing/data_abstraction.h>

LOG_MODULE_REGISTER(data_buffer, CONFIG_APP_LOG_LEVEL);

static K_THREAD_STACK_DEFINE(buffer_thread_stack_area, BUFFER_THREAD_STACK_SIZE);
struct k_sem data_in_buffer;

// Functions that bufferizes data in separate thread
static void perform_insert_in_buffer(void *, void *, void *);

int insert_in_buffer(){
    //Thread control block - metadata
	struct k_thread buffer_thread_data;
	k_tid_t buffer_thread_id;
    k_sem_init(&data_in_buffer, 0, 1);

	/* Create thread and start it immediately. */
	buffer_thread_id = k_thread_create(&buffer_thread_data, buffer_thread_stack_area,
							 K_THREAD_STACK_SIZEOF(buffer_thread_stack_area),
                             perform_insert_in_buffer, NULL, NULL, NULL, 
                             BUFFER_THREAD_PRIORITY, 0, K_NO_WAIT);
    return 0;
}

static void perform_insert_in_buffer(void *param0, void *param1, void *param2){
    ARG_UNUSED(param0);
    ARG_UNUSED(param1);
	ARG_UNUSED(param2);

    while(1){

        LOG_DBG("waiting data");
        //Waits indefinitely until sensor measurements are ready
        k_sem_take(&sensors_return.data_ready, K_FOREVER);
        LOG_DBG("data ready");

        // TODO: acquires lock and saves data to buffer

        // Notify buffer has data with semaphore
        k_sem_give(&data_in_buffer);
    }
}
