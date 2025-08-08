#include <stdio.h>
#include <scd30/scd30_setup.h>
#include <bme280/bme280_setup.h>

int main(void)
{
	init_scd30();
	init_bme280();
	
	int error = enable_scd30_low_power_mode();
	if (error)
	{
		printf("Error enabling SCD30 low power mode: %d\n", error);
		return error;
	}
	return 0;
}
