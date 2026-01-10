#pragma once

#include <stdint.h>
#include <zephyr/usb/usbd.h>

/*
 * This function configures and initializes a USB device. It configures the
 * device context, string descriptors, USB device configuration,
 * registers class instances, and finally initializes the USB device.
 * It is limited to a single device with a single configuration instantiated.
 *
 * It returns the configured and initialized USB device context on success,
 * otherwise it returns NULL.
 */
struct usbd_context *vinkey_usbd_init_device(usbd_msg_cb_t msg_cb);

/*
 * This function is similar to vinkey_usbd_init_device(), but does not
 * initialize the device. It allows the application to set additional features,
 * such as additional descriptors.
 */
struct usbd_context *vinkey_usbd_setup_device(usbd_msg_cb_t msg_cb);

