#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>


/**
 * DEFINITIONS
 */

// We need to create a new custom service as our data (characteristics)
// don't meet any existent Bluetooth service
#define BT_UUID_SERVICE_PULGA \
	BT_UUID_128_ENCODE(0x50756C67, 0x0000, 0x6158, 0x5376, 0x634D4F727369)
static const struct bt_uuid_128 pulga_service_uuid = BT_UUID_INIT_128(
	BT_UUID_SERVICE_PULGA);

// Buffers to store characteristics before sending
#define BUFFER_MAX_LEN 20
static uint8_t temperature_buffer[BUFFER_MAX_LEN];

/**
 * BLE Read handling
 */
static ssize_t read_temperature(struct bt_conn *connection, const struct bt_gatt_attr *attribute,
			        void *buffer, uint16_t length, uint16_t offset);

/** 
 * BLE Indicate handling
 */
// Controls whether server will indicate characteristics to client
static uint8_t indicate_on;
// If indication is happening (starts on bt_gatt_indicate, ends on destroy)
static uint8_t indicating;
// Indicate parameters
static struct bt_gatt_indicate_params indicate_params;
// Activates/deactivates BLE Indicate
static void pulga_ccc_cfg_changed(const struct bt_gatt_attr *attribute, uint16_t value);
// Callback of BLE Indicate, logs error or success
static void indicate_cb(struct bt_conn *connection, struct bt_gatt_indicate_params *params, 
    uint8_t error);
// Completes BLE Indicate
static void indicate_destroy(struct bt_gatt_indicate_params *params);

/**
 * Service management
 */
// // Runs after BT initialization and starts services
// static void init_services(void);
// Handles services updates on data
static int services_notify(void);

/**
 * Pulga Service Declaration
 */
BT_GATT_SERVICE_DEFINE(pulga_service, 
    BT_GATT_PRIMARY_SERVICE(&pulga_service_uuid),
    // Temperature Characteristic
    BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
        // Allows Read (client requests data) or indication (server pushes data)
        BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
        // // Requires encryption for read access
        // BT_GATT_PERM_READ_ENCRYPT,
        // // Requires encryption on an authenticated link for read access
        // BT_GATT_PERM_READ_AUTHEN,
        // Permits reading unencrypted data
        BT_GATT_PERM_READ,
        read_temperature, NULL, temperature_buffer),
    // Client characteristic configuration descriptor 
    // Allows turning on/off communication started by server (indicate, notify)
    BT_GATT_CCC(pulga_ccc_cfg_changed, BT_GATT_PERM_READ), 
);

/**
 * IMPLEMENTATIONS
 */

/*
** BLE Read handling
*/
static ssize_t read_temperature(struct bt_conn *connection, const struct bt_gatt_attr *attribute,
                    void *buffer, uint16_t length, uint16_t offset){
    const char *temp_value = attribute->user_data;

	return bt_gatt_attr_read(connection, attribute, buffer, length, offset, temp_value,
                strlen(temperature_buffer));
}

/*
** BLE Indicate handling
*/
// Activates/deactivates BLE Indicate
static void pulga_ccc_cfg_changed(const struct bt_gatt_attr *attribute, uint16_t value){
    indicate_on = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}
// Callback of BLE Indicate, logs error or success
static void indicate_cb(struct bt_conn *connection, struct bt_gatt_indicate_params *params, 
    uint8_t error){
    printk("Indication %s\n", error != 0U ? "fail" : "success");
}
// Completes BLE Indicate
static void indicate_destroy(struct bt_gatt_indicate_params *params){
    printk("Indication complete\n");
	indicating = 0U;
}

// Handling battery service data
static void bas_notify(void){
    uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

// Handles services updates on data
static int services_notify(void){
    bas_notify();

    char str[BT_UUID_STR_LEN];
    // Attribute to indicate
    struct bt_gatt_attr *ind_attr = bt_gatt_find_by_uuid(pulga_service.attrs, pulga_service.attr_count,
		BT_UUID_TEMPERATURE);
	bt_uuid_to_str(BT_UUID_TEMPERATURE, str, sizeof(str));
	printk("Indicate VND attr %p (UUID %s)\n", ind_attr, str);
    
    if (indicate_on && ind_attr) {
        if (indicating) {
            return -1;
        }

        indicate_params.attr = ind_attr;
        indicate_params.func = indicate_cb;
        indicate_params.destroy = indicate_destroy;
        indicate_params.data = &indicating;
        indicate_params.len = sizeof(indicating);

        if (bt_gatt_indicate(NULL, &indicate_params) == 0) {
            indicating = 1U;
        }
	}
    return 0;
}