#include <zephyr/logging/log.h>
#include <integration/data_abstraction/abstraction_service.h>
#include <integration/data_abstraction/text_model/text_model.h>
#include <sensors/sensors_interface.h>

LOG_MODULE_REGISTER(data_abstraction, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

// List of registered data APIs not corresponding to sensor data
static DataAPI *data_apis[SENSOR_TYPE_OFFSET] = {0};

/**
 * IMPLEMENTATIONS
 */

int register_data_callbacks()
{
	data_apis[TEXT_DATA] = register_text_model_callbacks();
	return 0;
}

// Encodes data to specified format
int encode_data(uint32_t *data_words, enum DataType data_type, enum EncodingLevel encoding,
				uint8_t *encoded_data, size_t encoded_size)
{
	// Gets correct data API corresponding to given data type
	DataAPI *data_api = get_data_api(data_type);
	switch (encoding)
	{
	case VERBOSE:
		return data_api->encode_verbose(data_words, encoded_data, encoded_size);
		break;
	case MINIMALIST:
		return data_api->encode_minimalist(data_words, encoded_data, encoded_size);
		break;
	case RAW_BYTES:
		return data_api->encode_raw_bytes(data_words, encoded_data, encoded_size);
		break;
	case ZCBOR:
		return data_api->encode_zcbor(data_words, encoded_data, encoded_size);
		break;
	default:
		LOG_ERR("Invalid encoding level");
		return -EINVAL;
	}
}

DataAPI *get_data_api(enum DataType data_type)
{
	if (data_type >= SENSOR_TYPE_OFFSET)
	{
		// Removes offset and searches sensor APIs, that's why
		// order of sensors and sensors data types needs to be the same
		data_type = data_type - SENSOR_TYPE_OFFSET;
		return sensor_apis[data_type]->data_model_api;
	}
	return data_apis[data_type];
}