#ifndef TIMESTAMP_SERVICE_H
#define TIMESTAMP_SERVICE_H

#include <zephyr/kernel.h>

// Get the current timestamp in seconds
uint32_t get_current_timestamp();

#define GPS_EPOCH_TO_POSIX(expr) (expr + 315964800)

#ifndef CONFIG_EVENT_TIMESTAMP_UPTIME
// Set the synchronization time in seconds
void set_sync_time_seconds(uint32_t sync_real_time);
#endif /* CONFIG_EVENT_TIMESTAMP_UPTIME */

#endif /* TIMESTAMP_SERVICE_H */