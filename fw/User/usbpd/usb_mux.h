//
// Copyright 2024 Wenting Zhang <zephray@outlook.com>
//
/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* USB mux driver */
#ifndef __CROS_EC_USB_MUX_H
#define __CROS_EC_USB_MUX_H

#include "usb_pd.h"

/* USB-C mux state */
typedef uint8_t mux_state_t;

enum typec_mux {
	TYPEC_MUX_NONE = 0,                /* Open switch */
	TYPEC_MUX_USB,  /* USB only */
	TYPEC_MUX_DP,   /* DP only */
	TYPEC_MUX_DOCK  /* Both USB and DP */
};

enum usb_switch {
	USB_SWITCH_CONNECT,
	USB_SWITCH_DISCONNECT,
	USB_SWITCH_RESTORE,
};

/**
 * Initialize USB mux to its default state.
 *
 * @param port  Port number.
 */
void usb_mux_init(int port);

/**
 * Configure superspeed muxes on type-C port.
 *
 * @param port port number.
 * @param mux_mode mux selected function.
 * @param usb_config usb2.0 selected function.
 * @param polarity plug polarity (0=CC1, 1=CC2).
 */
void usb_mux_set(int port, enum typec_mux mux_mode,
		 enum usb_switch usb_config, int polarity);

#endif