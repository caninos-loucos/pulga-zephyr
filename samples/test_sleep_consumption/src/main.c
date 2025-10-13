#include <zephyr/types.h>
#include <string.h>

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

int main(void)
{
	printf("Starting application");
	//k_sleep(K_MSEC(1000));

	uint32_t aux, aux_time;

    while(1){
		printf("Starting sum");

		aux = 0;
		aux_time = k_uptime_get();

		while(k_uptime_get() - aux_time < 10000){


			for (uint32_t i = 0; i < 100000; i++){
				aux++;
			}
		}

		printf("Starting sleep");

		k_sleep(K_MSEC(10000));

		printf("Starting busy wait");

		k_busy_wait(10000000);
	}

	k_sleep(K_FOREVER);
	return 0;
}