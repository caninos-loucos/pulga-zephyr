#include <zephyr/bluetooth/bluetooth.h>

static uint8_t temperature_buffer[10];

// We need to create a new custom service as our data (characteristics)
// don't meet any existent Bluetooth service
#define BT_UUID_SERVICE_PULGA \
	BT_UUID_128_ENCODE(0x50756C67, 0x0000, 0x6158, 0x5376, 0x634D4F727369)

static const struct bt_uuid_128 pulga_service_uuid = BT_UUID_INIT_128(
	BT_UUID_SERVICE_PULGA);

static const struct bt_uuid_128 vnd_enc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

static const struct bt_uuid_128 vnd_auth_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2));

#define VND_MAX_LEN 20

static ssize_t read_temperature(struct bt_conn *connection, const struct bt_gatt_attr *attribute,
			        void *buffer, uint16_t length, uint16_t offset);
