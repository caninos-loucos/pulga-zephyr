#ifndef TIMESTAMP_SERVICE_H
#define TIMESTAMP_SERVICE_H

#include <zephyr/kernel.h>

uint32_t get_current_timestamp();

#ifndef CONFIG_EVENT_TIMESTAMP_UPTIME
void set_sync_time_seconds(uint32_t sync_real_time);
#endif /* CONFIG_EVENT_TIMESTAMP_UPTIME */

#endif /* TIMESTAMP_SERVICE_H */