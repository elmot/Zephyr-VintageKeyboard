#pragma once

#include <stdint.h>

void vinkey_ble_init(void);
void vinkey_ble_send_report(const uint8_t *report, uint16_t len);
