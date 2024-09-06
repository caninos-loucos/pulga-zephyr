#ifndef DATA_ABSTRACTION_H
#define DATA_ABSTRACTION_H

#include <zephyr/kernel.h>

/*
 * Refer to:
 * https://docs.zephyrproject.org/latest/kernel/services/threads/index.html
 */
#define BUFFER_THREAD_STACK_SIZE 512
#define BUFFER_THREAD_PRIORITY 5 /* preemptible */
extern struct k_sem data_in_buffer;

// Inserts data in ring buffer
int insert_in_buffer();

#endif /* DATA_ABSTRACTION_H */