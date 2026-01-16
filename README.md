# Elmot's Vintage Keyboard

This project is a Zephyr-based firmware for converting vintage typewriters (like the Brother AX110) into modern USB and Bluetooth Low Energy (BLE) HID devices. 
It uses the Zephyr RTOS to handle USB device stacks, BLE advertising, and input matrix scanning.

## Hardware

**Board:** Nordic Semiconductor nRF52840-DK 

**GPIO extender:** Semtech SX1509B

| Component         | Pin                 |
|-------------------|---------------------|
| **I2C SCL**       | **P0.30**           |
| **I2C SDA**       | **P0.31**           |
| **NUM LOCK LED**  | **LED1**, **P0.14** |
| **CAPS LOCK LED** | **LED2**, **P0.13** |

## Key Features
- **Dual Connectivity**: Supports both USB HID and BLE HID.
- **Dynamic Key Mapping**: Translates raw scan codes into standard HID key codes.
- **Modifier Support**: Handles standard modifiers (Shift, Ctrl, Alt) and special function keys.
- **Blue Alt Mode**: A custom function layer activated by a specific key.

## Keys Functionality

The key mapping is defined in `src/ax110keys.c`. It maps raw 16-bit scan codes to HID key codes.

## Blue Alt Layer

When the **<span style="color:#4682B4">ALT</alt>** key is pressed and held, several keys change their functionality to provide common modern keyboard features that might be missing from the vintage layout.

| Original Key (Nordic layout)                           | Function       | Blue <span style="color:#4682B4">ALT</span> Function | 
|--------------------------------------------------------|----------------|------------------------------------------------------|
| **<span style="color:green">L IND</span>**             | **TAB**        | **ESC**                                              | 
| **<span style="color:green">CODE</span>**              | **Left CTRL**  | **Left CTRL**                                        | 
| **1,2,..., 0**                                         | **1,2,..., 0** | **F1 - F10**                                         | 
| Top row **?,`**                                        | **/,`*         | **F11, F12**                                         | 
| **RELOC**                                              | **PG UP**      | **HOME**                                             | 
| **INDEX/<span style="color:green">REV</span>**         | **PG DOWN**    | **END**                                              | 
| **W**                                                  | **W**          | **UP**                                               | 
| **A**                                                  | **A**          | **LEFT**                                             | 
| **S**                                                  | **S**          | **DOWN**                                             | 
| **D**                                                  | **D**          | **RIGHT**                                            | 
| **WORD OUT/<span style="color:green">LINE OUT</span>** | **ALT**        | **ALT**                                              | 

## Build and Flash

The project is built using the Zephyr build system (west).

```bash
west build -b <board_name>
west flash
```

Designed to run on boards like the `nrf52840dk_nrf52840`.
