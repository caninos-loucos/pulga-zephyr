#include <zephyr/logging/log.h>
#include <data_processing/data_abstraction.h>
#include <data_processing/si1133/si1133_model.h>

LOG_MODULE_REGISTER(data_abstraction, CONFIG_APP_LOG_LEVEL);

/**
 * DEFINITIONS
 */

DataAPI* data_apis[MAX_DATA_TYPE] = {0};

/**
 * IMPLEMENTATIONS
 */

int register_data_callbacks(){
	LOG_DBG("Registering data abstraction callbacks");
	// #if defined(CONFIG_BME280)
	// sensors_apis[BME280_MODEL] = register_bme280_model_callbacks;
	// #endif /* CONFIG_BME280 */

	// #if defined(CONFIG_BMI160)
	// sensors_apis[BMI160_MODEL] = register_bmi160_model_callbacks;
	// #endif /* CONFIG_BMI160 */

	#if defined(CONFIG_SI1133)
	data_apis[SI1133_MODEL] = register_si1133_model_callbacks();
	#endif /* CONFIG_SI1133 */

	// #if defined(CONFIG_SCD30)
	// sensors_apis[SCD30_MODEL] = register_scd30_model_callbacks;
	// #endif /* CONFIG_SCD30 */

	return 0;
}