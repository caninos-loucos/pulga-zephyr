#include <zephyr/logging/log.h>
#include <data_processing/data_abstraction.h>
#include <sensors/sensors_interface.h>

LOG_MODULE_REGISTER(data_abstraction, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

DataAPI *data_apis[SENSOR_TYPE_OFFSET] = {0};

/**
 * IMPLEMENTATIONS
 */

int register_data_callbacks()
{
	return 0;
}

// Encode data to specified format
void encode_data(enum DataType data_type, enum EncodingLevel encoding,
				 uint32_t *data_model, uint8_t *encoded_data, size_t encoded_size)
{
	switch (encoding)
	{
	case VERBOSE:
		get_data_api(data_type)->encode_verbose(data_model, encoded_data, encoded_size);
		break;
	default:
		LOG_ERR("Invalid encoding level");
	}
}

DataAPI *get_data_api(enum DataType data_type)
{
	if (data_type >= SENSOR_TYPE_OFFSET)
	{
		data_type = data_type - SENSOR_TYPE_OFFSET;
		return sensors_apis[data_type]->sensor_model_api;
	}
	return data_apis[data_type];
}