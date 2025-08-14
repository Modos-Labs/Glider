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

#define RX_BLK_SIZE (64*1024)

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // This example doesn't use multiple report and report ID
    (void) instance;
    (void) report_id;
    (void) report_type;

    // Process the request
    uint8_t cmd = buffer[0];
    uint16_t param = (buffer[2] << 8) | buffer[1];
    uint16_t x0 = (buffer[4] << 8) | buffer[3];
    uint16_t y0 = (buffer[6] << 8) | buffer[5];
    uint16_t x1 = (buffer[8] << 8) | buffer[7];
    uint16_t y1 = (buffer[10] << 8) | buffer[9];
    uint16_t id = (buffer[12] << 8) | buffer[11];
    uint16_t chksum = (buffer[14] << 8) | buffer[13];

    static bool is_recv = false;
    static uint16_t recv_name_cnt;
    static uint32_t recv_data_cnt;
    static uint32_t recv_buf_cnt;
    static uint32_t recv_chksum_expected;
    static uint32_t recv_chksum;
    static spiffs_file recv_f;
    static uint8_t *recv_buf;

    uint8_t retval = 1;
    uint16_t exp_chksum;
    bool ret = true;

    if (!is_recv) {
        exp_chksum = crc16(buffer, 13);
        if (chksum != exp_chksum) {
            retval = USBRET_CHKSUMFAIL;
            goto returnval;
        }
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
            retval = caster_redraw(x0, y0, x1, y1);
            break;
        case USBCMD_SETMODE:
            retval = caster_setmode(x0, y0, x1, y1, (update_mode_t)param);
            break;
        case USBCMD_USBBOOT:
            //iap_usbboot();
            break;
        case USBCMD_NUKE:
            //iap_nuke();
            break;
        case USBCMD_RECV:
            is_recv = true;
            recv_name_cnt = param;
            recv_data_cnt = ((uint32_t)y0 << 16) | (uint32_t)x0;
            recv_chksum_expected = ((uint32_t)y1 << 16) | (uint32_t)x1;
            recv_buf_cnt = 0;
            recv_buf = pvPortMalloc(RX_BLK_SIZE);
            retval = 0;
            break;
        }
    }
    else {
        ret = false;
        //exp_chksum = crc16(buffer, 16);
        if (recv_name_cnt > 0) {
            // Buffer should be a null terminated string
            recv_f = SPIFFS_open(&spiffs_fs, buffer, SPIFFS_O_CREAT | SPIFFS_O_TRUNC | SPIFFS_O_WRONLY, 0);
            recv_name_cnt = 0;
            ret = true;
            syslog_printf("Start receiving file %s, %d bytes\n", buffer, recv_data_cnt);
        }
        else if (recv_data_cnt > 0) {
            uint8_t data_to_recv = (recv_data_cnt > bufsize) ? bufsize : recv_data_cnt;
            // More to recv, recv all 16 bytes into buffer
            //syslog_dump_bytes(buffer, bufsize);
            //syslog_printf("RCNT: %d, CCNT: %d\n", bufsize, data_to_recv);
            memcpy(recv_buf + recv_buf_cnt, buffer, data_to_recv);
            recv_buf_cnt += data_to_recv;
            recv_data_cnt -= data_to_recv;
            if ((recv_buf_cnt >= (RX_BLK_SIZE - 64)) || (recv_data_cnt == 0)) {
                SPIFFS_write(&spiffs_fs, recv_f, recv_buf, recv_buf_cnt);
                recv_buf_cnt = 0;
                if (recv_data_cnt == 0) {
                    SPIFFS_close(&spiffs_fs, recv_f);
                    is_recv = false;
                    vPortFree(recv_buf);
                    ret = true;
                    syslog_printf("File received\n");
                }
            }
        }
        retval = 0;
    }

    if (retval == 0)
        retval = USBRET_SUCCESS;
    else
        retval = USBRET_GENERALFAIL;

returnval:
    uint8_t txbuf[CFG_TUD_HID_EP_BUFSIZE] = {0};
    txbuf[1] = retval;
    txbuf[2] = buffer[13];
    txbuf[3] = buffer[14];
    txbuf[4] = exp_chksum & 0xff;
    txbuf[5] = (exp_chksum >> 8) & 0xff;

    if (ret)
        tud_hid_report(0, txbuf, CFG_TUD_HID_EP_BUFSIZE);
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

