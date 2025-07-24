#include <stdio.h>
#include <scd30/scd30_calibration.h>

int main(void)
{
	init_scd30();
	int error = enable_scd30_low_power_mode();
	if (error)
	{
		printf("Error enabling SCD30 low power mode: %d\n", error);
		return error;
	}
	return 0;
}
