//
// Copyright 2024 Wenting Zhang <zephray@outlook.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "platform.h"
#include "board.h"
#include "app.h"
#include "usb_mux.h"

void usb_mux_init(int port) {
    //
}

void usb_mux_set(int port, enum typec_mux mux_mode,
		 enum usb_switch usb_config, int polarity) {
    syslog_printf("USB MUX set %s, %s, %d",
        (mux_mode == TYPEC_MUX_NONE) ? "NONE" : 
        (mux_mode == TYPEC_MUX_USB) ? "USB" :
        (mux_mode == TYPEC_MUX_DP) ? "DP" : "DOCK",
        (usb_config == USB_SWITCH_CONNECT) ? "CONNECT" :
        (usb_config == USB_SWITCH_DISCONNECT) ? "DISCONNECT" : "RESTORE",
        polarity);
    
    if (usb_config == USB_SWITCH_CONNECT) {
        if (polarity == 0) {
            // Not flipped
            syslog_printf("Setting orientation to not flipped");
        }
        else {
            // Flipped
            syslog_printf("Setting orientation to flipped");
        }
        gpio_put(TYPEC_ORI, polarity ^ TYPEC_MB_ORI_INV);
        ptn3460_set_aux_polarity(polarity ^ TYPEC_AUX_ORI_INV);
    }
}
