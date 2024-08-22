
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// Work responsible for advertising device
static struct k_work work_adv_start;
// Number of connections - expected to change anytime
static uint8_t volatile conn_count;
static uint8_t id_current;
static bool volatile is_disconnecting;

// Data to be sent in advertisement packets
static const struct bt_data adv_data[] = {
	// General discoverable enabled, enhanced data rate disabled
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

// Data to be sent in Scan Response packets (when central asks for more info)
static const struct bt_data scan_resp_data[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// Start advertising
static void adv_start(struct k_work *work)
{
	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		// Each adv set can have different parameters
		.sid = 0,
		.secondary_max_skip = 0,
		// Advertises as connectable and stops advertising
		// as connectable after one peripheral connection
		.options = (BT_LE_ADV_OPT_CONNECTABLE |
					BT_LE_ADV_OPT_ONE_TIME),
		.interval_min = 0x0020, /* 20 ms */
		.interval_max = 0x0060, /* 60 ms */
		.peer = NULL,
	};
	size_t id_count = 0xFF;
	int err;

	// id_count = count of all available and
	// existent identities, 0 at the start
	bt_id_get(NULL, &id_count);

	// No available existent IDs
	if (id_current == id_count)
	{
		int id;

		// Tries to create a new identity between
		// 0 and CONFIG_BT_MAX_CONN
		id = bt_id_create(NULL, NULL);
		if (id < 0)
		{
			printk("Create id failed (%d)\n", id);
			// Ensures id isn't negative
			if (id_current == 0)
			{
				id_current = CONFIG_BT_MAX_CONN;
			}
			id_current--;
		}
		else
		{
			printk("New id: %d\n", id);
		}
	}

	printk("Using current id: %u\n", id_current);
	adv_param.id = id_current;

	err = bt_le_adv_start(&adv_param, adv_data, ARRAY_SIZE(adv_data),
						  scan_resp_data, ARRAY_SIZE(scan_resp_data));
	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	// Ensures id isn't bigger than maximum number of connections
	id_current++;
	if (id_current == CONFIG_BT_MAX_CONN)
	{
		id_current = 0;
	}

	printk("Advertising successfully started\n");
}

// Callback function when device tries to connect
static void connected(struct bt_conn *conn, uint8_t err)
{
	char address[BT_ADDR_LE_STR_LEN];

	// Converts error code to string and prints it
	if (err)
	{
		printk("Connection failed, error 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	conn_count++;
	// If the device can accept more connections, start advertising again
	if (conn_count < CONFIG_BT_MAX_CONN)
	{
		k_work_submit(&work_adv_start);
	}

	// Gets address of connected device and converts it to string
	bt_addr_le_to_str(bt_conn_get_dst(conn), address, sizeof(address));

	printk("Connected (%u): %s\n", conn_count, address);
}

// Callback function when device disconnects
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char address[BT_ADDR_LE_STR_LEN];

	// Gets address of connected device and converts it to string
	bt_addr_le_to_str(bt_conn_get_dst(conn), address, sizeof(address));

	// Converts error code to string and prints it
	printk("Disconnected %s, reason %s(0x%02x)\n", address, bt_hci_err_to_str(reason), reason);

	// Waits until all connections are dropped to start advertising again
	if ((conn_count == 1U) && is_disconnecting)
	{
		is_disconnecting = false;
		k_work_submit(&work_adv_start);
	}
	conn_count--;
}

// Callback function that logs and accepts peer requests to change connection parameters
// https://punchthrough.com/manage-ble-connection/
static bool ble_param_request(struct bt_conn *conn, struct bt_le_conn_param *new_params)
{
	char address[BT_ADDR_LE_STR_LEN];

	// Gets address of connected device and converts it to string
	bt_addr_le_to_str(bt_conn_get_dst(conn), address, sizeof(address));

	// Latency: how many times device can ignore connection events before
	//          handling one, saving energy
	// Timeout: time after which connection is considered to be lost
	printk("BLE connection new parameters request: address \"%s\" \
		   interval (0x%04x, 0x%04x) latency %d timeout %d\n",
		   address, new_params->interval_min,
		   new_params->interval_max, new_params->latency, new_params->timeout);

	return true;
}

// Callback function that logs changes to connection parameters
static void ble_param_updated(struct bt_conn *conn, uint16_t interval,
							  uint16_t latency, uint16_t timeout)
{
	char address[BT_ADDR_LE_STR_LEN];

	// Gets address of connected device and converts it to string
	bt_addr_le_to_str(bt_conn_get_dst(conn), address, sizeof(address));

	printk("BLE connection parameters updated: address \"%s\" interval \
			0x%04x latency %d timeout %d\n",
		   address, interval, latency, timeout);
}

// Only compiled if application has access to change physical layer parameters
#if defined(CONFIG_BT_USER_PHY_UPDATE)
static void ble_phy_updated(struct bt_conn *conn,
			   struct bt_conn_le_phy_info *param)
{
	char address[BT_ADDR_LE_STR_LEN];

	// Gets address of connected device and converts it to string
	bt_addr_le_to_str(bt_conn_get_dst(conn), address, sizeof(address));

	// PHY translates to the bit rate on the physical layer
	printk("BLE PHY Updated: %s Tx 0x%x, Rx 0x%x\n", address, param->tx_phy,
	       param->rx_phy);
}
#endif /* CONFIG_BT_USER_PHY_UPDATE */

// Only compiled if application has access to change size and duration
// of data packets (payload) on link layer
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static void ble_data_len_updated(struct bt_conn *conn,
				struct bt_conn_le_data_len_info *info)
{
	char address[BT_ADDR_LE_STR_LEN];

	// Gets address of connected device and converts it to string
	bt_addr_le_to_str(bt_conn_get_dst(conn), address, sizeof(address));

	// Logs maximum lenght of packets and expected maximum duration
	// of transmission. Set lower values for stable connections 
	printk("Data length updated: %s max tx %u bytes (%u us) max rx %u bytes (%u us)\n",
	       address, info->tx_max_len, info->tx_max_time, info->rx_max_len,
	       info->rx_max_time);
}
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */

// Only compiled if Security Manager Protocol, responsible for providing
// authentication, authorization and encryption, is activated
#if defined(CONFIG_BT_SMP)

// Logs changes in security parameters of a connection or any 
// errors during the changes
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char address[BT_ADDR_LE_STR_LEN];

	// Gets address of connected device and converts it to string
	bt_addr_le_to_str(bt_conn_get_dst(conn), address, sizeof(address));

	if (!err) {
		printk("Security changed: %s level %u\n", address, level);
	} else {
		// Converts error code to string and prints it
		printk("Security failed: %s level %u err %s(%d)\n", address, level,
		       bt_security_err_to_str(err), err);
	}
}

// Logs when pairing process is cancelled
static void auth_cancel(struct bt_conn *conn)
{
	char address[BT_ADDR_LE_STR_LEN];

	// Gets address of connected device and converts it to string
	bt_addr_le_to_str(bt_conn_get_dst(conn), address, sizeof(address));

	printk("Pairing cancelled: %s\n", address);
}

// Registers struct contaning callback functions 
// related to authentication events
static struct bt_conn_auth_cb auth_callbacks = {
	.cancel = auth_cancel,
};
#endif /* CONFIG_BT_SMP */

// Registers struct containing callback functions 
// related to Bluetooth connections
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = ble_param_request,
	.le_param_updated = ble_param_updated,

#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	.le_phy_updated = le_phy_updated,
#endif /* CONFIG_BT_USER_PHY_UPDATE */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	.le_data_len_updated = le_data_len_updated,
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */
};

// Initialization of Bluetooth Low Energy peripheral device
int init_peripheral(uint8_t iterations)
{
	size_t id_count;
	int err;

	// Enables bluetooth
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	// Registers connection callbacks
	bt_conn_cb_register(&conn_callbacks);

#if defined(CONFIG_BT_SMP)
	// Registers authentication callbacks
	bt_conn_auth_cb_register(&auth_callbacks);
#endif /* CONFIG_BT_SMP */

	printk("Bluetooth initialized\n");

	// Initializes work responsible for start advertising 
	k_work_init(&work_adv_start, adv_start);
	// Submits advertising work to the queue for the first time
	k_work_submit(&work_adv_start);

	/* wait for connection attempts on all identities */
	do {
		k_sleep(K_MSEC(10));

		id_count = 0xFF;
		bt_id_get(NULL, &id_count);
	} while (id_count != CONFIG_BT_MAX_CONN);

	/* rotate identities so reconnections are attempted in case of any
	 * disconnections
	 */
	uint8_t prev_count = conn_count;
	while (1) {
		/* If maximum connections is reached, wait for disconnections
		 * initiated by peer central devices.
		 */
		if (conn_count == CONFIG_BT_MAX_CONN) {
			if (!iterations) {
				break;
			}
			iterations--;
			printk("Iterations remaining: %u\n", iterations);

			printk("Wait for disconnections...\n");
			is_disconnecting = true;
			while (is_disconnecting) {
				k_sleep(K_MSEC(10));
			}
			printk("All disconnected.\n");

			continue;
		}

		/* As long as there is connection count changes, identity
		 * rotation in this loop is not needed.
		 */
		if (prev_count != conn_count) {
			prev_count = conn_count;

			continue;
		} else {
			uint16_t wait = 6200U;

			/* Maximum duration without connection count change,
			 * central waiting before disconnecting all its
			 * connections plus few seconds of margin.
			 */
			while ((prev_count == conn_count) && wait) {
				wait--;

				k_sleep(K_MSEC(10));
			}

			if (wait) {
				continue;
			}
		}

		/* Stopping and restarting advertising to use new identity */
		printk("Stop advertising...\n");
		err = bt_le_adv_stop();
		if (err) {
			printk("Failed to stop advertising (%d)\n", err);

			return err;
		}

		k_work_submit(&work_adv_start);
	}

	return 0;
}