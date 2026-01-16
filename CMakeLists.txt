cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(Zephyr-VintageKeyboard)

target_sources(app PRIVATE src/main.c src/vinkey_usbd_init.c src/vinkey_ble.c src/ax110keys.c)

