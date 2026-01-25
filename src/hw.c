#include "main.h"

const struct gpio_dt_spec caps_lock_led = GPIO_DT_SPEC_GET(DT_ALIAS(caps_lock_led), gpios);
const struct gpio_dt_spec pwr_on_led = GPIO_DT_SPEC_GET(DT_ALIAS(pwr_on_led), gpios);
const struct gpio_dt_spec ble_connected_led = GPIO_DT_SPEC_GET(DT_ALIAS(ble_connected_led), gpios);
const struct gpio_dt_spec usb_connected_led = GPIO_DT_SPEC_GET(DT_ALIAS(usb_connected_led), gpios);

LOG_MODULE_REGISTER(hw, LOG_LEVEL_INF);

static void busy_wait_cycles(uint32_t cycles)
{
    // Simple busy-wait loop
    // Each iteration takes approximately 4 cycles (load, decrement, compare, branch)
    for (volatile uint32_t i = 0; i < cycles / 4; i++)
    {
        __asm__ volatile ("nop");
    }
}

static void busy_wait_ms(uint32_t ms)
{
    // At 32MHz: 32,000 cycles per millisecond
    uint32_t cycles = ms * 32000;
    busy_wait_cycles(cycles);
}

_Noreturn void failure()
{
    while (true)
    {
        gpio_pin_set_dt(&pwr_on_led, true);
        gpio_pin_set_dt(&usb_connected_led, false);
        gpio_pin_set_dt(&ble_connected_led, false);

        busy_wait_ms(250);
        gpio_pin_set_dt(&pwr_on_led, false);
        gpio_pin_set_dt(&usb_connected_led, true);
        gpio_pin_set_dt(&ble_connected_led, false);
        busy_wait_ms(500);
        gpio_pin_set_dt(&pwr_on_led, false);
        gpio_pin_set_dt(&usb_connected_led, false);
        gpio_pin_set_dt(&ble_connected_led, true);
        busy_wait_ms(250);
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
static void ensure_voltage(unsigned long voltage) {
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


void init_hardware()
{
    ensure_voltage(UICR_REGOUT0_VOUT_3V0);
    prepare_led(&pwr_on_led, true);
    prepare_led(&caps_lock_led, false);
    prepare_led(&ble_connected_led, false);
    prepare_led(&usb_connected_led, false);

}

void update_connect_status()
{
    gpio_pin_set_dt(&usb_connected_led, usb_kb_ready);
    gpio_pin_set_dt(&ble_connected_led, ble_kb_ready);
    gpio_pin_set_dt(&pwr_on_led, !(ble_kb_ready || usb_kb_ready));
}

_Noreturn void arch_system_halt(unsigned int reason)
{
    ARG_UNUSED(reason);
    failure();
}
