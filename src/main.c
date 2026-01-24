/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vinkey_usbd.h"
#include "vinkey_ble.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_hid.h>

#include <zephyr/logging/log.h>
#include <hal/nrf_power.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include <zephyr/dt-bindings/input/input-event-codes.h>

extern uint8_t input_to_hid(uint16_t code, int32_t value);
extern bool is_modifier(uint16_t code);


static const struct gpio_dt_spec caps_lock_led = GPIO_DT_SPEC_GET(DT_ALIAS(caps_lock_led), gpios);
static const struct gpio_dt_spec pwr_on_led = GPIO_DT_SPEC_GET(DT_ALIAS(pwr_on_led), gpios);
static const struct gpio_dt_spec ble_connected_led = GPIO_DT_SPEC_GET(DT_ALIAS(ble_connected_led), gpios);
static const struct gpio_dt_spec usb_connected_led = GPIO_DT_SPEC_GET(DT_ALIAS(usb_connected_led), gpios);

enum kb_report_idx {
	KB_MOD_KEY = 0,
	KB_RESERVED,
	KB_KEY_CODE1,
	KB_KEY_CODE2,
	KB_KEY_CODE3,
	KB_KEY_CODE4,
	KB_KEY_CODE5,
	KB_KEY_CODE6,
	KB_REPORT_COUNT,
};

struct kb_event {
	uint16_t code;
	int32_t value;
};

K_MSGQ_DEFINE(kb_msgq, sizeof(struct kb_event), 2, 1);

UDC_STATIC_BUF_DEFINE(report, KB_REPORT_COUNT);

static const uint8_t hid_report_desc[] = HID_KEYBOARD_REPORT_DESC();


static void update_report(uint16_t code, int32_t value)
{
	uint8_t hid_code = input_to_hid(code, value);
	if (hid_code == 0) {
		return;
	}

	if (is_modifier(code)) {
		if (value) {
			report[KB_MOD_KEY] |= hid_code;
		} else {
			report[KB_MOD_KEY] &= ~hid_code;
		}
	} else {
		if (value) {
			/* Add to report */
			for (int i = KB_KEY_CODE1; i < KB_REPORT_COUNT; i++) {
				if (report[i] == 0) {
					report[i] = hid_code;
					break;
				}
			}
		} else {
			/* Remove from report */
			for (int i = KB_KEY_CODE1; i < KB_REPORT_COUNT; i++) {
				if (report[i] == hid_code) {
					report[i] = 0;
					/* Shift remaining keys left */
					for (int j = i; j < KB_REPORT_COUNT - 1; j++) {
						report[j] = report[j+1];
					}
					report[KB_REPORT_COUNT - 1] = 0;
					break;
				}
			}
		}
	}
}
static uint32_t kb_duration;
volatile static bool usb_kb_ready = false;

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
			struct kb_event kb_evt = {
				.code = code,
				.value = evt->value,
			};
			if (k_msgq_put(&kb_msgq, &kb_evt, K_NO_WAIT) != 0) {
				LOG_ERR("Failed to put new input event");
			}
		}
	}
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

void update_connect_status()
{
	gpio_pin_set_dt(&usb_connected_led, usb_kb_ready);
	gpio_pin_set_dt(&ble_connected_led, ble_kb_ready);
	gpio_pin_set_dt(&pwr_on_led, !(ble_kb_ready || usb_kb_ready));
	}

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

/* doc device msg-cb start */
static void msg_cb(struct usbd_context *const usbd_ctx,
		   const struct usbd_msg *const msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (msg->type == USBD_MSG_CONFIGURATION) {
		LOG_INF("\tConfiguration value %d", msg->status);
	}

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(usbd_ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(usbd_ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}
}

/**
 * @brief Ensures that the output voltage is set to the specified value.
 *
 * This method checks whether the REGOUT0 configuration corresponds to the specified voltage.
 * If the configuration differs, it reconfigures the voltage setting in the UICR (User Information Configuration Registers),
 * programs the change, and resets the system to apply the new setting.
 *
 * Available UICR_REGOUT0_VOUT values:
 * - UICR_REGOUT0_VOUT_1V8  (0) - 1.8V
 * - UICR_REGOUT0_VOUT_2V1  (1) - 2.1V
 * - UICR_REGOUT0_VOUT_2V4  (2) - 2.4V
 * - UICR_REGOUT0_VOUT_2V7  (3) - 2.7V
 * - UICR_REGOUT0_VOUT_3V0  (4) - 3.0V
 * - UICR_REGOUT0_VOUT_3V3  (5) - 3.3V
 *
 * @param voltage The desired output voltage to be configured. The value should match the UICR register's expected format.
 */
void ensure_voltage(unsigned long voltage) {
	// Check if REGOUT0 is NOT set to 3.3V (Value 5)
	if ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) !=
		(voltage << UICR_REGOUT0_VOUT_Pos))
	{
		// Enable Write to NVMC
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}

		// Set REGOUT0 to 3.0V
		NRF_UICR->REGOUT0 = (NRF_UICR->REGOUT0 & ~UICR_REGOUT0_VOUT_Msk) |
							(voltage << UICR_REGOUT0_VOUT_Pos);

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}

		// A System Reset is required for the change to take effect
		NVIC_SystemReset();
	}
}

static int prepare_led(const struct gpio_dt_spec* led, bool active)
{
	if (!gpio_is_ready_dt(led))
	{
		LOG_ERR("LED device %s is not ready", led->port->name);
		return -EIO;
	}

	int ret = gpio_pin_configure_dt(led, active ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE);
	if (ret != 0)
	{
		LOG_ERR("Failed to configure the %s pin, %d", led->port->name, ret);
		return -EIO;
	}
	return 0;
}

_Noreturn void failure()
{
	while (true) {
		gpio_pin_set_dt(&pwr_on_led, true);
		gpio_pin_set_dt(&usb_connected_led, false);
		gpio_pin_set_dt(&ble_connected_led, false);
		k_sleep(K_MSEC(250));
		gpio_pin_set_dt(&pwr_on_led, false);
		gpio_pin_set_dt(&usb_connected_led, true);
		gpio_pin_set_dt(&ble_connected_led, false);
		k_sleep(K_MSEC(500));
		gpio_pin_set_dt(&pwr_on_led, false);
		gpio_pin_set_dt(&usb_connected_led, false);
		gpio_pin_set_dt(&ble_connected_led, true);
		k_sleep(K_MSEC(250));
	}
}
int main(void)
{
	ensure_voltage(UICR_REGOUT0_VOUT_3V0);
	prepare_led(&pwr_on_led, true);
	prepare_led(&caps_lock_led, false);
	prepare_led(&ble_connected_led, false);
	prepare_led(&usb_connected_led, false);
	const struct device* hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
	if (!device_is_ready(hid_dev)) {
		LOG_ERR("HID Device is not ready");
		failure();
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
	struct usbd_context* vinkey_usbd = vinkey_usbd_init_device(msg_cb);
	if (vinkey_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		failure();
	}

	if (!usbd_can_detect_vbus(vinkey_usbd)) {
		/* doc device enable start */
		ret = usbd_enable(vinkey_usbd);
		if (ret) {
			LOG_ERR("Failed to enable device support");
			failure();
		}
		/* doc device enable end */
	}

	LOG_INF("HID keyboard is initialized");

	// ReSharper disable once CppDFAEndlessLoop
	while (true) {
		struct kb_event kb_evt;

		k_msgq_get(&kb_msgq, &kb_evt, K_FOREVER);

		uint8_t hid_code = input_to_hid(kb_evt.code, kb_evt.value);
		vinkey_ble_handle_key(hid_code, kb_evt.value);

		update_report(kb_evt.code, kb_evt.value);

		vinkey_ble_send_report(report, KB_REPORT_COUNT);

		if (!usb_kb_ready) {
//			LOG_INF("USB HID device is not ready");
			continue;
		}

		ret = hid_device_submit_report(hid_dev, KB_REPORT_COUNT, report);
		if (ret) {
//			LOG_ERR("HID submit report error, %d", ret);
		}
	}

}
