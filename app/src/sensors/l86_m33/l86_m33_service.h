#ifndef L86_M33_SERVICE_H
#define L86_M33_SERVICE_H

#include <zephyr/drivers/gnss.h>
#include <sensors/sensors_interface.h>

// Number of 32-bit words in each data item (model)
#define GNSS_MODEL_WORDS 9

typedef struct
{
    /** Navigation data acquired */
	struct navigation_data navigation; // 7 words
	/** UTC time when data was acquired */
	struct gnss_time real_time; // 2 words
    
} SensorModelGNSS;

// Registers Si1133 model callbacks
DataAPI *register_gnss_model_callbacks();

// Registers Si1133 sensor callbacks
SensorAPI *register_l86_m33_callbacks();

#endif /* L86_M33_SERVICE_H */