#ifndef _GNSS_QUECTEL_L86_H_
#define _GNSS_QUECTEL_L86_H_

#include <zephyr/drivers/sensor.h>

/* functions to be shared in application and driver */

int quectel_l86_enter_backup_mode(const struct device *dev);

void quectel_l86_exit_backup_mode(const struct device *dev);

#endif // _GNSS_QUECTEL_L86_H_