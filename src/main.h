#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_hid.h>

#include <zephyr/logging/log.h>

extern const struct gpio_dt_spec caps_lock_led;
extern const struct gpio_dt_spec pwr_on_led;
extern const struct gpio_dt_spec ble_connected_led;
extern const struct gpio_dt_spec usb_connected_led;

_Noreturn void failure();
void init_hardware();

typedef void (*vinkey_ble_output_report_cb_t)(const uint8_t *report, uint16_t len);
void vinkey_ble_init();
void vinkey_ble_send_report(const uint8_t *report, uint16_t len);
void vinkey_ble_handle_key(uint8_t hid_code, bool pressed);

extern volatile bool ble_kb_ready;
extern volatile bool usb_kb_ready;

void update_connect_status();

int kb_set_report(const struct device *dev,
             uint8_t type, uint8_t id, uint16_t len,
             const uint8_t * buf);

uint8_t input_to_hid(uint16_t code, int32_t value);
bool is_modifier(uint16_t code);

void vinkey_usb_init();
