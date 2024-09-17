#ifndef BME280_SERVICE_H
#define BME280_SERVICE_H

#include <sensors/sensors_interface.h>

// Register BME280 sensor callbacks
SensorAPI* register_bme280_callbacks();

#endif /* BME280_SERVICE_H */