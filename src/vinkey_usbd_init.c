/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_vinkey_config);

/* By default, do not register the USB DFU class DFU mode instance. */
static const char *const blocklist[] = {
	"dfu_dfu",
	NULL,
};

#define CONFIG_VINKEY_USBD_VID (0x16C0)
#define CONFIG_VINKEY_USBD_PID (0x27DB)
#define CONFIG_VINKEY_USBD_MANUFACTURER "Elmot.xyz"
#define CONFIG_VINKEY_USBD_PRODUCT "Elmot Vintage Kbd (Brother AX110 Mod) USB"
#define CONFIG_SAMPLE_USBD_MAX_POWER (125)

/* doc device instantiation start */
/*
 * Instantiate a context named vinkey_usbd using the default USB device
 * controller, the project vendor ID, and the product ID.
 */
USBD_DEVICE_DEFINE(vinkey_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   CONFIG_VINKEY_USBD_VID, CONFIG_VINKEY_USBD_PID);
/* doc device instantiation end */

/* doc string instantiation start */
USBD_DESC_LANG_DEFINE(vinkey_lang);
USBD_DESC_MANUFACTURER_DEFINE(vinkey_mfr, CONFIG_VINKEY_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(vinkey_product, CONFIG_VINKEY_USBD_PRODUCT);
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(vinkey_sn)));

/* doc string instantiation end */

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

/* doc configuration instantiation start */
static const uint8_t attributes = (0);

/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(vinkey_fs_config,
			  attributes,
			  CONFIG_SAMPLE_USBD_MAX_POWER, &fs_cfg_desc);

/* High speed configuration */
USBD_CONFIGURATION_DEFINE(vinkey_hs_config,
			  attributes,
			  CONFIG_SAMPLE_USBD_MAX_POWER, &hs_cfg_desc);
/* doc configuration instantiation end */

static void vinkey_fix_code_triple(struct usbd_context *uds_ctx,
				   const enum usbd_speed speed)
{
	/* Always use class code information from Interface Descriptors */
	if (IS_ENABLED(CONFIG_USBD_CDC_ACM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_ECM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_CDC_NCM_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_MIDI2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_AUDIO2_CLASS) ||
	    IS_ENABLED(CONFIG_USBD_VIDEO_CLASS)) {
		/*
		 * Class with multiple interfaces have an Interface
		 * Association Descriptor available, use an appropriate triple
		 * to indicate it.
		 */
		usbd_device_set_code_triple(uds_ctx, speed,
					    USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	} else {
		usbd_device_set_code_triple(uds_ctx, speed, 0, 0, 0);
	}
}

struct usbd_context *vinkey_usbd_setup_device(usbd_msg_cb_t msg_cb)
{
	int err;

	/* doc add string descriptor start */
	err = usbd_add_descriptor(&vinkey_usbd, &vinkey_lang);
	if (err) {
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&vinkey_usbd, &vinkey_mfr);
	if (err) {
		LOG_ERR("Failed to initialize manufacturer descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&vinkey_usbd, &vinkey_product);
	if (err) {
		LOG_ERR("Failed to initialize product descriptor (%d)", err);
		return NULL;
	}

	IF_ENABLED(CONFIG_HWINFO, (
		err = usbd_add_descriptor(&vinkey_usbd, &vinkey_sn);
	))
	if (err) {
		LOG_ERR("Failed to initialize SN descriptor (%d)", err);
		return NULL;
	}
	/* doc add string descriptor end */

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&vinkey_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&vinkey_usbd, USBD_SPEED_HS,
					     &vinkey_hs_config);
		if (err) {
			LOG_ERR("Failed to add High-Speed configuration");
			return NULL;
		}

		err = usbd_register_all_classes(&vinkey_usbd, USBD_SPEED_HS, 1,
						blocklist);
		if (err) {
			LOG_ERR("Failed to add register classes");
			return NULL;
		}

		vinkey_fix_code_triple(&vinkey_usbd, USBD_SPEED_HS);
	}

	/* doc configuration register start */
	err = usbd_add_configuration(&vinkey_usbd, USBD_SPEED_FS,
				     &vinkey_fs_config);
	if (err) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return NULL;
	}
	/* doc configuration register end */

	/* doc functions register start */
	err = usbd_register_all_classes(&vinkey_usbd, USBD_SPEED_FS, 1, blocklist);
	if (err) {
		LOG_ERR("Failed to add register classes");
		return NULL;
	}
	/* doc functions register end */

	vinkey_fix_code_triple(&vinkey_usbd, USBD_SPEED_FS);
	usbd_self_powered(&vinkey_usbd, attributes & USB_SCD_SELF_POWERED);

	if (msg_cb != NULL) {
		/* doc device init-and-msg start */
		err = usbd_msg_register_cb(&vinkey_usbd, msg_cb);
		if (err) {
			LOG_ERR("Failed to register message callback");
			return NULL;
		}
		/* doc device init-and-msg end */
	}

	return &vinkey_usbd;
}

struct usbd_context *vinkey_usbd_init_device(usbd_msg_cb_t msg_cb)
{
	int err;

	if (vinkey_usbd_setup_device(msg_cb) == NULL) {
		return NULL;
	}

	/* doc device init start */
	err = usbd_init(&vinkey_usbd);
	if (err) {
		LOG_ERR("Failed to initialize device support");
		return NULL;
	}
	/* doc device init end */

	return &vinkey_usbd;
}
