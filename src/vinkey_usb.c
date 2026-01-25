/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main.h"
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_vinkey_config);

/* By default, do not register the USB DFU class DFU mode instance. */
static const char* const blocklist[] = {
    "dfu_dfu",
    NULL,
};

#define CONFIG_VINKEY_USBD_VID (CONFIG_BT_DIS_PNP_VID)
#define CONFIG_VINKEY_USBD_PID (CONFIG_BT_DIS_PNP_PID)
#define CONFIG_SAMPLE_USBD_MAX_POWER (125)

/*
 * Instantiate a context named vinkey_usbd using the default USB device
 * controller, the project vendor ID, and the product ID.
 */
USBD_DEVICE_DEFINE(vinkey_usbd,
                   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   CONFIG_VINKEY_USBD_VID, CONFIG_VINKEY_USBD_PID);

USBD_DESC_LANG_DEFINE(vinkey_lang);
USBD_DESC_MANUFACTURER_DEFINE(vinkey_mfr, CONFIG_BT_DIS_MANUF_NAME_STR);
USBD_DESC_PRODUCT_DEFINE(vinkey_product, CONFIG_BT_DEVICE_NAME "[USB]");
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(vinkey_sn)));

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

static const uint8_t attributes = (0);

/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(vinkey_fs_config,
                          attributes,
                          CONFIG_SAMPLE_USBD_MAX_POWER, &fs_cfg_desc);

/* High speed configuration */
USBD_CONFIGURATION_DEFINE(vinkey_hs_config,
                          attributes,
                          CONFIG_SAMPLE_USBD_MAX_POWER, &hs_cfg_desc);

static void vinkey_fix_code_triple(struct usbd_context* uds_ctx,
                                   const enum usbd_speed speed)
{
    usbd_device_set_code_triple(uds_ctx, speed, 0, 0, 0);
}

volatile bool usb_kb_ready = false;

static void msg_cb(struct usbd_context* const usbd_ctx,
                   const struct usbd_msg* const msg)
{
    LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

    if (msg->type == USBD_MSG_CONFIGURATION)
    {
        LOG_INF("\tConfiguration value %d", msg->status);
    }

    if (usbd_can_detect_vbus(usbd_ctx))
    {
        if (msg->type == USBD_MSG_VBUS_READY)
        {
            if (usbd_enable(usbd_ctx))
            {
                LOG_ERR("Failed to enable device support");
            }
        }

        if (msg->type == USBD_MSG_VBUS_REMOVED)
        {
            if (usbd_disable(usbd_ctx))
            {
                LOG_ERR("Failed to disable device support");
            }
        }
    }
}

/*
 * This function is similar to vinkey_usbd_init_device(), but does not
 * initialize the device. It allows the application to set additional features,
 * such as additional descriptors.
 */
struct usbd_context* vinkey_usbd_setup_device()
{
    int err;

    err = usbd_add_descriptor(&vinkey_usbd, &vinkey_lang);
    if (err)
    {
        LOG_ERR("Failed to initialize language descriptor (%d)", err);
        return NULL;
    }

    err = usbd_add_descriptor(&vinkey_usbd, &vinkey_mfr);
    if (err)
    {
        LOG_ERR("Failed to initialize manufacturer descriptor (%d)", err);
        return NULL;
    }

    err = usbd_add_descriptor(&vinkey_usbd, &vinkey_product);
    if (err)
    {
        LOG_ERR("Failed to initialize product descriptor (%d)", err);
        return NULL;
    }

    IF_ENABLED(CONFIG_HWINFO, (
                   err = usbd_add_descriptor(&vinkey_usbd, &vinkey_sn);
               ))
    if (err)
    {
        LOG_ERR("Failed to initialize SN descriptor (%d)", err);
        return NULL;
    }

    if (USBD_SUPPORTS_HIGH_SPEED &&
        usbd_caps_speed(&vinkey_usbd) == USBD_SPEED_HS)
    {
        err = usbd_add_configuration(&vinkey_usbd, USBD_SPEED_HS,
                                     &vinkey_hs_config);
        if (err)
        {
            LOG_ERR("Failed to add High-Speed configuration");
            return NULL;
        }

        err = usbd_register_all_classes(&vinkey_usbd, USBD_SPEED_HS, 1,
                                        blocklist);
        if (err)
        {
            LOG_ERR("Failed to add register classes");
            return NULL;
        }

        vinkey_fix_code_triple(&vinkey_usbd, USBD_SPEED_HS);
    }

    err = usbd_add_configuration(&vinkey_usbd, USBD_SPEED_FS,
                                 &vinkey_fs_config);
    if (err)
    {
        LOG_ERR("Failed to add Full-Speed configuration");
        return NULL;
    }

    err = usbd_register_all_classes(&vinkey_usbd, USBD_SPEED_FS, 1, blocklist);
    if (err)
    {
        LOG_ERR("Failed to add register classes");
        return NULL;
    }

    vinkey_fix_code_triple(&vinkey_usbd, USBD_SPEED_FS);
    usbd_self_powered(&vinkey_usbd, attributes & USB_SCD_SELF_POWERED);

    err = usbd_msg_register_cb(&vinkey_usbd, msg_cb);
    if (err)
    {
        LOG_ERR("Failed to register message callback");
        return NULL;
    }

    return &vinkey_usbd;
}


/*
 * This function configures and initializes a USB device. It configures the
 * device context, string descriptors, USB device configuration,
 * registers class instances, and finally initializes the USB device.
 * It is limited to a single device with a single configuration instantiated.
 *
 * It returns the configured and initialized USB device context on success,
 * otherwise it returns NULL.
 */
void vinkey_usbd_init_device()
{
    if (vinkey_usbd_setup_device() == NULL)
    {
        failure();
    }
    if (usbd_init(&vinkey_usbd))
    {
        LOG_ERR("Failed to initialize device support");
        failure();
    }
}

void vinkey_usb_init()
{
    vinkey_usbd_init_device();

    if (!usbd_can_detect_vbus(&vinkey_usbd))
    {
        if (usbd_enable(&vinkey_usbd))
        {
            LOG_ERR("Failed to enable device support");
            failure();
        }
    }
}
