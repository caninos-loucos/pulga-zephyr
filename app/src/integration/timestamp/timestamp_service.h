#ifndef TIMESTAMP_SERVICE_H
#define TIMESTAMP_SERVICE_H

#include <zephyr/kernel.h>

uint64_t get_current_timestamp();

#ifndef CONFIG_EVENT_TIMESTAMP_UPTIME
void set_startup_time(uint32_t sync_time);
#endif /* CONFIG_EVENT_TIMESTAMP_UPTIME */

#endif /* TIMESTAMP_SERVICE_H */