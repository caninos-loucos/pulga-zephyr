#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>
#include <integration/timestamp/timestamp_service.h>

LOG_MODULE_REGISTER(timestamp_service, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

#ifndef CONFIG_EVENT_TIMESTAMP_UPTIME
static uint64_t startup_time;
#endif /* CONFIG_EVENT_TIMESTAMP_UPTIME */

/**
 * IMPLEMENTATIONS
 */

uint64_t get_current_timestamp()
{
#if defined(CONFIG_EVENT_TIMESTAMP_UPTIME)
   return k_uptime_seconds();
#else
   return k_uptime_seconds() + startup_time;
#endif
}

#ifndef CONFIG_EVENT_TIMESTAMP_UPTIME
void set_startup_time(uint32_t sync_time)
{
   startup_time = sync_time - k_uptime_seconds();
}
#endif /* CONFIG_EVENT_TIMESTAMP_UPTIME */