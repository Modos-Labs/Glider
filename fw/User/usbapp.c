//
// Grimoire
// Copyright 2025 Wenting Zhang
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
#include "app.h"
#include "tusb.h"

void usbapp_term_out(char data, void *usr) {
	tud_cdc_write_char(data);
    tud_cdc_write_flush();
}

QueueHandle_t rxqueue;

// TODO: Implement proper RTOS wakeup instead of this wait 10ms thing
void tud_cdc_rx_cb(uint8_t itf) {
    // There is a risk where the queue is full, not all bytes would be moved
    // to the RTOS queue. The remaining bytes won't be moved until there is new
    // stuff coming in from the CDC.
    while ((uxQueueSpacesAvailable(rxqueue) > 0) && (tud_cdc_available())) {
        uint8_t c = tud_cdc_read_char();
        xQueueSend(rxqueue, &c, 0);
    }
}

int usbapp_term_in(int mode, void *usr) {
    uint8_t c;

    int timeout = 0;
    if (mode != TERM_INPUT_DONT_WAIT)
        timeout = pdMS_TO_TICKS(mode + 1);
    BaseType_t result = xQueueReceive(rxqueue, &c, timeout);
    if (result == pdTRUE)
        return c;
    else
        return -1;
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

    uint8_t retval;
    uint16_t exp_chksum = crc16(buffer, 14);
    if (chksum != exp_chksum) {
        retval = USBRET_CHKSUMFAIL;
        goto returnval;
    }
    retval = 1;

    switch (cmd) {
    case USBCMD_RESET:
        // reset system
        //iap_reset();
        break;
    case USBCMD_POWERDOWN:
        // TODO
        break;
    case USBCMD_POWERUP:
        // TODO
        break;
    case USBCMD_SETINPUT:
        // TODO
        break;
    case USBCMD_REDRAW:
        //retval = caster_redraw(x0, y0, x1, y1);
        break;
    case USBCMD_SETMODE:
        //retval = caster_setmode(x0, y0, x1, y1, (UPDATE_MODE)param);
        break;
    case USBCMD_USBBOOT:
        //iap_usbboot();
        break;
    case USBCMD_NUKE:
        //iap_nuke();
        break;
    }
    if (retval == 0)
        retval = USBRET_SUCCESS;
    else
        retval = USBRET_GENERALFAIL;

returnval:
    uint8_t txbuf[16] = {0};
    txbuf[0] = retval;
    txbuf[2] = buffer[14];
    txbuf[3] = buffer[15];
    txbuf[4] = exp_chksum & 0xff;
    txbuf[5] = (exp_chksum >> 8) & 0xff;

    tud_hid_report(0, txbuf, 16);
}

portTASK_FUNCTION(usb_device_task, pvParameters) {
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    
    rxqueue = xQueueCreate(1024, sizeof(char));
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    // RTOS forever loop
    while (1) {
        // put this thread to waiting state until there is new events
        tud_task();

        // following code only run if tud_task() process at least 1 event
    }
}

