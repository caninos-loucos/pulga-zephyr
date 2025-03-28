#ifndef L86_M33_SERVICE_H
#define L86_M33_SERVICE_H

#include <zephyr/drivers/gnss.h>
#include <sensors/sensors_interface.h>
#include <integration/data_buffer/buffer_service.h>
typedef struct
{
	int32_t latitude; // microdegrees (0 to +- 180E6)
	int32_t longitude; // microdegrees (0 to +- 180E6)
	uint16_t bearing; // centidegrees (0 - 36000)
	uint16_t speed; // cm/s, up to 650m/s
	uint32_t altitude; // cm, up to the stratosphere (65km)
	struct gnss_time real_time;
	uint32_t timestamp; // 1 word
} SensorModelGNSS;

// Number of 32-bit words in each data item (model)
// Padding added in navigation field to align
// by largest member (64-bit fields)
#define GNSS_MODEL_WORDS SIZE_BYTES_TO_32_BIT_WORDS(sizeof(SensorModelGNSS))

// Registers Si1133 model callbacks
DataAPI *register_gnss_model_callbacks();

// Registers Si1133 sensor callbacks
SensorAPI *register_l86_m33_callbacks();

#endif /* L86_M33_SERVICE_H */