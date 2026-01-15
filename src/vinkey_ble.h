#pragma once

#include <stdint.h>

typedef void (*vinkey_ble_output_report_cb_t)(const uint8_t *report, uint16_t len);
void vinkey_ble_init();
void vinkey_ble_send_report(const uint8_t *report, uint16_t len);

int kb_set_report(const struct device *dev,
             uint8_t type, uint8_t id, uint16_t len,
             const uint8_t * buf);