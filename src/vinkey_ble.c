#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>


#include "vinkey_ble.h"

#include "zephyr/usb/class/usbd_hid.h"

LOG_MODULE_REGISTER(vinkey_ble, LOG_LEVEL_INF);

#define HIDS_NORMALLY_CONNECTABLE BIT(1)

struct hids_info {
	uint16_t version;
	uint8_t code;
	uint8_t flags;
} __packed;

struct hids_report {
	uint8_t id;
	uint8_t type;
} __packed;

static struct hids_info info = {
	.version = 0x0111,
	.code = 0x00,
	.flags = HIDS_NORMALLY_CONNECTABLE,
};

enum {
	HIDS_INPUT = 0x01,
	HIDS_OUTPUT = 0x02,
	HIDS_FEATURE = 0x03,
};

static struct hids_report input_report_ref = {
	.id = 0x00,
	.type = HIDS_INPUT,
};

static struct hids_report output_report_ref = {
	.id = 0x00,
	.type = HIDS_OUTPUT,
};

static const uint8_t report_map[] = HID_KEYBOARD_REPORT_DESC();

static ssize_t read_info(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_map,
				 sizeof(report_map));
}

static ssize_t read_report_ref(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct hids_report));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_INF("HIDS input CCC changed: %u", value);
}

static ssize_t read_input_report(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_output_report(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len, uint16_t offset,
				  uint8_t flags)
{
	LOG_INF("HIDS output report write: len %u", len);

	kb_set_report(NULL, HID_REPORT_TYPE_OUTPUT, 0, len, buf);

	return len;
}

BT_GATT_SERVICE_DEFINE(kbd_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_info, NULL, &info),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_report_map, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ_ENCRYPT,
			       read_input_report, NULL, NULL),
	BT_GATT_CCC(input_ccc_changed,
		    BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
			   read_report_ref, NULL, &input_report_ref),
	BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ_ENCRYPT |
			       BT_GATT_PERM_WRITE_ENCRYPT,
			       read_input_report, write_output_report, NULL),
	BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
			   read_report_ref, NULL, &output_report_ref),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,

			  (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
			  (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL)),
	BT_DATA(BT_DATA_NAME_SHORTENED, CONFIG_BT_NAME_SHORTENED, sizeof(CONFIG_BT_NAME_SHORTENED)-1)
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, (sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
};

static struct bt_conn *current_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Failed to connect to %s (%u)", addr, err);
		return;
	}

	LOG_INF("Connected %s", addr);
	current_conn = bt_conn_ref(conn);
}

bool advertising_start()
{
	bt_le_adv_stop();
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd,ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return true;
	}

	LOG_INF("Advertising successfully started");
	return false;
}


static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected from %s (reason %u)", addr, reason);
	if (current_conn == conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_ERR("Security failed: %s level %u err %d", addr, level, err);
	}
}

void conn_recycled()
{
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.recycled = conn_recycled,
};

static uint32_t passkey_entered;
static uint8_t passkey_digit_count;
static bool passkey_entry_mode;

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Enter passkey for %s", addr);
	passkey_entered = 0;
	passkey_digit_count = 0;
	passkey_entry_mode = true;
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
	passkey_entry_mode = false;
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = auth_passkey_entry,
	.cancel = auth_cancel,
};

void vinkey_ble_handle_key(uint8_t hid_code, bool pressed)
{
	if (!pressed || !passkey_entry_mode || !current_conn) {
		return;
	}

	if (hid_code >= HID_KEY_1 && hid_code <= HID_KEY_0) {
		/* Numbers 1-9 then 0 */
		uint8_t digit;
		if (hid_code == HID_KEY_0) {
			digit = 0;
		} else {
			digit = hid_code - HID_KEY_1 + 1;
		}

		if (passkey_digit_count < 6) {
			passkey_entered = passkey_entered * 10 + digit;
			passkey_digit_count++;
			LOG_INF("Passkey digit entered: %u (total: %u)", digit, passkey_entered);
		}
	} else if (hid_code == HID_KEY_ENTER) {
		/* Enter */
		LOG_INF("Passkey submitted: %06u", passkey_entered);
		bt_conn_auth_passkey_entry(current_conn, passkey_entered);
		passkey_entry_mode = false;
	} else if (hid_code == HID_KEY_BACKSPACE) {
		/* Backspace */
		if (passkey_digit_count > 0) {
			passkey_entered /= 10;
			passkey_digit_count--;
			LOG_INF("Passkey digit removed (total: %u)", passkey_entered);
		}
	}
}

void vinkey_ble_init()
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	bt_conn_auth_cb_register(&auth_cb_display);

	advertising_start();
}

void vinkey_ble_send_report(const uint8_t *report, uint16_t len)
{
	/* Index 6 is the report characteristic value in kbd_svc */
	bt_gatt_notify(NULL, &kbd_svc.attrs[6], report, len);
}
