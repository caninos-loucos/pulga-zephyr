#include <zephyr/types.h>
#include <string.h>

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define WAIT_PERIOD K_SECONDS(20)
int main(void)
{
	printf("Starting application");
	k_sleep(WAIT_PERIOD);

	uint32_t aux = 5;
	while (1)
	{
		printf("Starting sum");

		k_timepoint_t timeout = sys_timepoint_calc(WAIT_PERIOD);
		while (!sys_timepoint_expired(timeout))
		{
			aux += aux / 999;
			aux *= 19867 % 64546;
		}

		printf("Starting sleep");
		k_sleep(WAIT_PERIOD);

		printf("Starting busy wait");
		k_busy_wait(15/10 * WAIT_PERIOD.ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC * USEC_PER_SEC);

		printf("Starting sleep");
		k_sleep(WAIT_PERIOD);
	}

	k_sleep(K_FOREVER);
	return 0;
}
