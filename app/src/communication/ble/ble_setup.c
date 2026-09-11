#include <zephyr/logging/log.h>
#include <sensors/scd30/scd30_service.h>
#include <communication/ble/ble_setup.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>


// change log level in debug.conf
LOG_MODULE_REGISTER(ble_setup, CONFIG_APP_LOG_LEVEL);

int8_t rslt;

static uint8_t bt_connected, bt_advertising;
static uint8_t indicating, indicate_htm;
static struct bt_gatt_indicate_params ind_params;

static void htmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	indicate_htm = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
    LOG_DBG("ccc_indicate %s\n", indicate_htm == 0U ? "disabled" : "enabled");
}

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	if(err != 0U) LOG_DBG("Indication fail\n\r");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	LOG_DBG("Indication complete\n");
	indicating = 0U;
}

#define READING_SVC_UUID                 BT_UUID_128_ENCODE(0x02b454b7, 0xc19f, 0x4d1c, 0xa2c0, 0xb7fc10f8a8a3)
#define READING_CHR_IND_ACCEL_UUID      BT_UUID_128_ENCODE(0x682d75cf, 0x44fc, 0x4df4, 0xac81, 0x00f02aa9b98a)

static const struct bt_uuid_128 read_svc_uuid = BT_UUID_INIT_128(READING_SVC_UUID);
static const struct bt_uuid_128 read_char_ind_accel_uuid = BT_UUID_INIT_128(READING_CHR_IND_ACCEL_UUID);

#define CO2_REFERENCE_SVC_UUID                 BT_UUID_128_ENCODE(0x4c1e2038, 0x3a74, 0x4a36, 0x96ac, 0x69c813248c14)
#define CO2_REFERENCE_CHR_IND_ACCEL_UUID      BT_UUID_128_ENCODE(0x3f49d1f4, 0x3a19, 0x4ca0, 0xa191, 0xd5787cbf2919)

static const struct bt_uuid_128 refe_svc_uuid = BT_UUID_INIT_128(CO2_REFERENCE_SVC_UUID);
static const struct bt_uuid_128 refe_char_write_uuid = BT_UUID_INIT_128(CO2_REFERENCE_CHR_IND_ACCEL_UUID);

#define TEMP_REFERENCE_SVC_UUID                 BT_UUID_128_ENCODE(0x5b62dda0, 0xb7eb, 0x4b4d, 0xb958, 0x4dfb1117344e)
#define TEMP_REFERENCE_CHR_IND_ACCEL_UUID      BT_UUID_128_ENCODE(0xa77f94a0, 0xf7f5, 0x4c64, 0xb8bc, 0xd30297b3f4a4)

static const struct bt_uuid_128 temp_svc_uuid = BT_UUID_INIT_128(TEMP_REFERENCE_SVC_UUID);
static const struct bt_uuid_128 temp_char_write_uuid = BT_UUID_INIT_128(TEMP_REFERENCE_CHR_IND_ACCEL_UUID);

/* BLE Service Declaration */
BT_GATT_SERVICE_DEFINE(read_svc,
	BT_GATT_PRIMARY_SERVICE(&read_svc_uuid),
	BT_GATT_CHARACTERISTIC(&read_char_ind_accel_uuid.uuid, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(htmc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

BT_GATT_SERVICE_DEFINE(refe_svc,
	BT_GATT_PRIMARY_SERVICE(&refe_svc_uuid),
	BT_GATT_CHARACTERISTIC(&refe_char_write_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_co2_reference, NULL),
);

BT_GATT_SERVICE_DEFINE(temp_svc,
	BT_GATT_PRIMARY_SERVICE(&temp_svc_uuid),
	BT_GATT_CHARACTERISTIC(&temp_char_write_uuid.uuid, BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE, NULL, write_temp_reference, NULL),
);



/*static ssize_t read_blsc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &hrs_blsc,
				 sizeof(hrs_blsc));
}


static ssize_t write_vnd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset, u8_t flags)
{
    u8_t *value = attr->user_data;

    if (offset + len > sizeof(vnd_value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    
    memcpy(value + offset, buf, len);

    return len;
}

 */

void reading_indicate(char* buffer)
{
    if(indicate_htm){

        ind_params.attr = &read_svc.attrs[2];
        ind_params.func = indicate_cb;
        ind_params.destroy = indicate_destroy;
        ind_params.data = buffer;
        ind_params.len = strlen(buffer);

        if (bt_gatt_indicate(NULL, &ind_params) != 0) {
        LOG_DBG("Indication fail\n\r");
        }
    }
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	LOG_DBG("%s: MTU exchange %s (%u)\n", __func__, err == 0U ? "successful" : "failed", bt_gatt_get_mtu(conn));
}

static struct bt_gatt_exchange_params mtu_exchange_params = {
	.func = mtu_exchange_cb
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HTS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
        bt_le_adv_stop();
	} else {
		LOG_DBG("Connected\n");

        bt_connected = 1;
        
        LOG_DBG("%s: Current MTU = %u\n", __func__, bt_gatt_get_mtu(conn));
        
        LOG_DBG("%s: Exchange MTU...\n", __func__);
	    err = bt_gatt_exchange_mtu(conn, &mtu_exchange_params);
	    if (err) {
		    LOG_DBG("%s: MTU exchange failed (err %d)", __func__, err);
	    }
    }
    bt_advertising = 0;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    bt_connected = 0;
	LOG_DBG("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

void bt_start_advertising(void)
{
	if(bt_connected || bt_advertising){
		return;
	}

	int err;

	LOG_DBG("Advertising start\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}

    bt_advertising = 1;

	LOG_DBG("Advertising successfully started\n");
}