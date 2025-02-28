#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>
#include <integration/timestamp/timestamp_service.h>

LOG_MODULE_REGISTER(timestamp_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

#ifndef CONFIG_EVENT_TIMESTAMP_UPTIME
static uint32_t sync_real_time_seconds = 0;
static uint32_t sync_uptime_seconds = 0;
#endif /* CONFIG_EVENT_TIMESTAMP_UPTIME */

/**
 * IMPLEMENTATIONS
 */

uint32_t get_current_timestamp()
{
#if defined(CONFIG_EVENT_TIMESTAMP_UPTIME)
   return k_uptime_seconds();
#else
   // Returns the updated timestamp taking the last synchronization time into account
   return sync_real_time_seconds + k_uptime_seconds() - sync_uptime_seconds;
#endif
}

#ifndef CONFIG_EVENT_TIMESTAMP_UPTIME
void set_sync_time_seconds(uint32_t sync_real_time)
{
   // Updates the synchronization time
   sync_real_time_seconds = sync_real_time;
   sync_uptime_seconds = k_uptime_seconds();
}
#endif /* CONFIG_EVENT_TIMESTAMP_UPTIME */