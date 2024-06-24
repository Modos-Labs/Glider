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
#include <stdio.h>
#include "pico/stdlib.h"
#include "fpga.h"
#include "caster.h"
#include "tusb.h"
#include "usb_descriptors.h"

void usbapp_init(void) {
    tusb_init();
}

// Device callbacks
void tud_mount_cb(void) {

}

void tud_umount_cb(void) {

}

void tud_suspend_cb(bool remote_wakeup_en) {
    // TODO: Force Suspend
}

void tud_resume_cb(void) {
    
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // This example doesn't use multiple report and report ID
    (void) instance;
    (void) report_id;
    (void) report_type;

    // Process the request
    uint16_t cmd = (buffer[1] << 8) | buffer[0];
    uint16_t param = (buffer[3] << 8) | buffer[2];
    uint16_t x0 = (buffer[5] << 8) | buffer[4];
    uint16_t y0 = (buffer[7] << 8) | buffer[6];
    uint16_t x1 = (buffer[9] << 8) | buffer[8];
    uint16_t y1 = (buffer[11] << 8) | buffer[10];
    uint16_t id = (buffer[13] << 8) | buffer[12];
    uint16_t chksum = (buffer[15] << 8) | buffer[14];
    uint8_t retval = 1;
    switch (buffer[0]) {
    case USBCMD_RESET:
        // reset system
        (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C))) = 0x5FA0004; // Reset via NVIC
        break;
    case USBCMD_POWERDOWN:
        // TODO
        break;
    case USBCMD_POWERUP:
        // TODO
        break;
    case USBCMD_SETINPUT:
        retval = caster_setinput((uint8_t)param);
        break;
    case USBCMD_REDRAW:
        retval = caster_redraw(x0, y0, x1, y1);
        break;
    case USBCMD_SETMODE:
        retval = caster_setmode(x0, y0, x1, y1, (UPDATE_MODE)param);
        break;
    }

    uint8_t txbuf[1];
    txbuf[0] = retval;

    tud_hid_report(0, txbuf, 1);
}

void usbapp_task(void) {
    tud_task();
}
