#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "comm_ble_service.h"

/* Pulga Service Declaration */
BT_GATT_SERVICE_DEFINE(pulga_service, BT_GATT_PRIMARY_SERVICE(&pulga_service_uuid),
    // Temperature Characteristic
    BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE_VAL,
        // Allows Read (client requests data) or indication (server pushes data)
        BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
        // // Requires encryption for read access
        // BT_GATT_PERM_READ_ENCRYPT,
        // Permits reading unencrypted data
        BT_GATT_PERM_READ,
        read_temperature, NULL, temperature_buffer),
    BT_GATT_CCC(ct_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), );


static ssize_t read_temperature(struct bt_conn *connection, const struct bt_gatt_attr *attribute,
                    void *buffer, uint16_t length, uint16_t offset){
    const char *temp_value = attr->user_data;

	return bt_gatt_attr_read(connection, attribute, buffer, length, offset, temp_value,
                strlen(temperature_buffer));
}