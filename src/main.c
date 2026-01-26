/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <zephyr/dt-bindings/input/input-event-codes.h>

#define KEYS_PER_REPORT (6)

struct kb_report {
	uint8_t modifier;
	uint8_t reserved;
	uint8_t keys[KEYS_PER_REPORT];
} __packed;

struct kb_event {
	uint16_t code;
	int32_t value;
};

K_MSGQ_DEFINE(usb_msgq, sizeof(struct kb_report), 10, 4);
K_MSGQ_DEFINE(ble_msgq, sizeof(struct kb_report), 10, 4);

static struct kb_report report;

static const uint8_t hid_report_desc[] = HID_KEYBOARD_REPORT_DESC();

const struct device* hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
const struct device* kscan_dev = DEVICE_DT_GET(DT_ALIAS(kscan));

static void update_report(uint16_t code, int32_t value)
{
	uint8_t hid_code = input_to_hid(code, value);
	if (hid_code == 0) {
		return;
	}

	if (is_modifier(code)) {
		if (value) {
			report.modifier |= hid_code;
		} else {
			report.modifier &= ~hid_code;
		}
	} else {
		if (value) {
			/* Add to report */
			for (int i = 0; i < KEYS_PER_REPORT; i++) {
				if (report.keys[i] == 0) {
					report.keys[i] = hid_code;
					break;
				}
			}
		} else {
			/* Remove from report */
			for (int i = 0; i < KEYS_PER_REPORT; i++) {
				if (report.keys[i] == hid_code) {
					report.keys[i] = 0;
					/* Shift remaining keys left */
					for (int j = i; j < (KEYS_PER_REPORT-1); j++) {
						report.keys[j] = report.keys[j+1];
					}
					report.keys[KEYS_PER_REPORT - 1] = 0;
					break;
				}
			}
		}
	}
}
static uint32_t kb_duration;

static int matrix_row = -1;
static int matrix_col = -1;

static void input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->code == INPUT_ABS_X) {
		matrix_col = evt->value;
	} else if (evt->code == INPUT_ABS_Y) {
		matrix_row = evt->value;
	} else if (evt->code == INPUT_BTN_TOUCH) {
		if (matrix_row >= 0 && matrix_col >= 0) {
			uint16_t code = (matrix_row << 8) | matrix_col;

			update_report(code, evt->value);

			k_msgq_put(&usb_msgq, &report, K_NO_WAIT);
			k_msgq_put(&ble_msgq, &report, K_NO_WAIT);
		}
	}
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

static void kb_iface_ready(const struct device *dev, const bool ready)
{
	LOG_INF("HID device %s interface is %s",
		dev->name, ready ? "ready" : "not ready");
	usb_kb_ready = ready;
	update_connect_status();
}

static int kb_get_report(const struct device *dev,
			 const uint8_t type, const uint8_t id, const uint16_t len,
			 uint8_t *const buf)
{
	LOG_WRN("Get Report not implemented, Type %u ID %u", type, id);

	return 0;
}

int kb_set_report(const struct device *dev,
			 const uint8_t type, const uint8_t id, const uint16_t len,
			 const uint8_t *const buf)
{
	if (type != HID_REPORT_TYPE_OUTPUT) {
		LOG_WRN("Unsupported report type");
		return -ENOTSUP;
	}

	gpio_pin_set_dt(&caps_lock_led, buf[0] & (int)BIT(1));
	return 0;
}

/* Idle duration is stored but not used to calculate idle reports. */
static void kb_set_idle(const struct device *dev,
			const uint8_t id, const uint32_t duration)
{
	LOG_INF("Set Idle %u to %u", id, duration);
	kb_duration = duration;
}

static uint32_t kb_get_idle(const struct device *dev, const uint8_t id)
{
	LOG_INF("Get Idle %u to %u", id, kb_duration);
	return kb_duration;
}

static void kb_set_protocol(const struct device *dev, const uint8_t proto)
{
	LOG_INF("Protocol changed to %s",
		proto == 0U ? "Boot Protocol" : "Report Protocol");
}

void kb_output_report(const struct device *dev, const uint16_t len,
			     const uint8_t *const buf)
{
	LOG_HEXDUMP_DBG(buf, len, "o.r.");
	kb_set_report(dev, HID_REPORT_TYPE_OUTPUT, 0U, len, buf);
}

struct hid_device_ops kb_ops = {
	.iface_ready = kb_iface_ready,
	.get_report = kb_get_report,
	.set_report = kb_set_report,
	.set_idle = kb_set_idle,
	.get_idle = kb_get_idle,
	.set_protocol = kb_set_protocol,
	.output_report = kb_output_report,
};

typedef int (*send_report_fn)(const uint8_t *report);

static _Noreturn void kb_usb_send_task(void *p1, void *p2, void *p3)
{
	static struct kb_report queued_report;

	while (true) {
		k_msgq_get(&usb_msgq, &queued_report, K_FOREVER);
		if (usb_kb_ready) {
			hid_device_submit_report(hid_dev, sizeof(struct kb_report), (uint8_t *)&queued_report);
		}
	}
}

static _Noreturn void kb_ble_send_task(void *p1, void *p2, void *p3)
{
	static struct kb_report queued_report;

	while (true) {
		k_msgq_get(&ble_msgq, &queued_report, K_FOREVER);
		vinkey_ble_send_report((uint8_t *)&queued_report, sizeof(struct kb_report));
	}
}

K_THREAD_DEFINE(usb_task_tid, 1024, kb_usb_send_task, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(ble_task_tid, 1024, kb_ble_send_task, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
	init_hardware();
	if (!device_is_ready(hid_dev)) {
		LOG_ERR("HID Device is not ready");
		failure();
	}

	if (!device_is_ready(kscan_dev)) {
		LOG_ERR("Kscan Device is not ready");
		key_scan_failure();
	}

	int ret = hid_device_register(hid_dev,
	                              hid_report_desc, sizeof(hid_report_desc),
	                              &kb_ops);
	if (ret != 0) {
		LOG_ERR("Failed to register HID Device, %d", ret);
		failure();
	}

	if (IS_ENABLED(CONFIG_USBD_HID_SET_POLLING_PERIOD)) {
		ret = hid_device_set_in_polling(hid_dev, 1000);
		if (ret) {
			LOG_WRN("Failed to set IN report polling period, %d", ret);
		}

		ret = hid_device_set_out_polling(hid_dev, 1000);
		if (ret != 0 && ret != -ENOTSUP) {
			LOG_WRN("Failed to set OUT report polling period, %d", ret);
		}
	}

	vinkey_ble_init();
	vinkey_usb_init();
	LOG_INF("HID keyboard is initialized");

	return 0;
}
