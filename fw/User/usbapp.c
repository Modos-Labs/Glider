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
#include "caster.h"
#include "ui.h"
#include "tusb.h"

// Mutex protecting config tone fields and the tone LUT from concurrent
// modification by the USB handler, the OSD UI task, and the shell.
static SemaphoreHandle_t usb_tone_lock;
static volatile bool usb_tone_lock_ready;

static bool usb_tone_lock_take(void) {
    if (!usb_tone_lock_ready)
        return false;
    return xSemaphoreTake(usb_tone_lock, pdMS_TO_TICKS(100)) == pdTRUE;
}

static void usb_tone_lock_give(void) {
    if (usb_tone_lock_ready)
        xSemaphoreGive(usb_tone_lock);
}

void usbapp_query_tone(int *lightness, int *contrast) {
    if (usb_tone_lock_take()) {
        *lightness = config.lightness;
        *contrast = config.contrast;
        usb_tone_lock_give();
    }
}

uint8_t usbapp_query_mode(void) {
    uint8_t mode = 0xFF;

    if (usb_tone_lock_take()) {
        mode = (uint8_t)config.update_mode;
        usb_tone_lock_give();
    }
    return mode;
}

uint8_t usbapp_query_signal_status(void) {
    return caster_input_status();
}

// Pending getter payloads delivered in the next IN report, matching the
// out-of-band response pattern used for command status.
static struct {
    int8_t lightness;
    int8_t contrast;
    bool pending;
} tone_getter;
static struct {
    uint8_t mode;
    bool pending;
} mode_getter;
static struct {
    uint8_t status;
    bool pending;
} signal_getter;
static struct {
    uint8_t mode;
    uint8_t interval;
    uint8_t threshold;
    bool pending;
} ac_getter;

void usbapp_term_out(char data, void *usr) {
	tud_cdc_write_char(data);
    tud_cdc_write_flush();
}

QueueHandle_t rxqueue;
static volatile bool usb_suspend_event;
static volatile bool usb_resume_event;

static bool usbapp_take_event(volatile bool *event) {
    bool pending;

    taskENTER_CRITICAL();
    pending = *event;
    *event = false;
    taskEXIT_CRITICAL();
    return pending;
}

static void usbapp_set_bus_suspend(bool suspended) {
    taskENTER_CRITICAL();
    usb_suspend_event = suspended;
    usb_resume_event = !suspended;
    taskEXIT_CRITICAL();
}

bool usbapp_take_suspend_event(void) {
    return usbapp_take_event(&usb_suspend_event);
}

bool usbapp_take_resume_event(void) {
    return usbapp_take_event(&usb_resume_event);
}

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

void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    usbapp_set_bus_suspend(true);
}

void tud_resume_cb(void) {
    usbapp_set_bus_suspend(false);
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

    // Only process CONTROL report. Other reports should not have SET_REPORT
    if (buffer[0] != REPORT_ID_CONTROL)
        return;

    // Process the request
    buffer++;
    bufsize -= 1;
    uint8_t cmd = buffer[0];
    uint16_t param = (buffer[2] << 8) | buffer[1];
    uint16_t x0 = (buffer[4] << 8) | buffer[3];
    uint16_t y0 = (buffer[6] << 8) | buffer[5];
    uint16_t x1 = (buffer[8] << 8) | buffer[7];
    uint16_t y1 = (buffer[10] << 8) | buffer[9];
    uint16_t id = (buffer[12] << 8) | buffer[11];
    uint16_t chksum = (buffer[14] << 8) | buffer[13];

    // Signed view of param for the tone commands (two's complement).
    int16_t param16 = (int16_t)param;

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
            power_off_epd();
            NVIC_SystemReset();
            break;
        case USBCMD_POWERDOWN:
            // TODO
            break;
        case USBCMD_POWERUP:
            // TODO
            break;
        case USBCMD_SETINPUT:
            config.input_sel = param;
            config_save();
            retval = 0;
            break;
        case USBCMD_REDRAW:
            retval = caster_redraw(x0, y0, x1, y1);
            break;
        case USBCMD_SETMODE:
            retval = caster_setmode(x0, y0, x1, y1, (update_mode_t)param);
            if (retval == 0) {
                // Track the mode in config so GETMODE reports USB-initiated
                // changes; the OSD path maintains this via apply_display_mode().
                config.update_mode = (int)param;
                config_request_save();
                usbapp_mode_changed = true;
            }
            break;
        case USBCMD_SETLIGHTNESS:
            if ((param16 < -3) || (param16 > 3)) {
                retval = USBRET_BADVALUE;
                goto returnval;
            }
            if (!usb_tone_lock_take()) {
                retval = USBRET_GENERALFAIL;
                goto returnval;
            }
            config.lightness = param16;
            // Remember this tone for the current mode, apply immediately via
            // the tone LUT (same path the OSD uses), and defer the flash
            // write to the UI task so the USB handler never does flash work
            // in its own context.
            config_note_tone_for_mode((update_mode_t)config.update_mode);
            config_request_save();
            caster_set_tone(config.lightness, config.contrast);
            usb_tone_lock_give();
            retval = 0;
            break;
        case USBCMD_SETCONTRAST:
            if ((param16 < -1) || (param16 > 6)) {
                retval = USBRET_BADVALUE;
                goto returnval;
            }
            if (!usb_tone_lock_take()) {
                retval = USBRET_GENERALFAIL;
                goto returnval;
            }
            config.contrast = param16;
            config_note_tone_for_mode((update_mode_t)config.update_mode);
            config_request_save();
            caster_set_tone(config.lightness, config.contrast);
            usb_tone_lock_give();
            retval = 0;
            break;
        case USBCMD_GETTONE:
            if (!usb_tone_lock_take()) {
                retval = USBRET_GENERALFAIL;
                goto returnval;
            }
            tone_getter.lightness = config.lightness;
            tone_getter.contrast = config.contrast;
            usb_tone_lock_give();
            tone_getter.pending = true;
            retval = 0;
            break;
        case USBCMD_GETMODE:
            mode_getter.mode = usbapp_query_mode();
            mode_getter.pending = true;
            retval = 0;
            break;
        case USBCMD_GETAC:
            ac_getter.mode = (uint8_t)config.autoclear_mode;
            ac_getter.interval = (uint8_t)config.autoclear_interval;
            ac_getter.threshold = (uint8_t)config.autoclear_threshold;
            ac_getter.pending = true;
            retval = 0;
            break;
        case USBCMD_GETSIGNAL:
            signal_getter.status = usbapp_query_signal_status();
            signal_getter.pending = true;
            retval = 0;
            break;
        case USBCMD_SETACMODE:
            if ((param16 < AC_OFF) || (param16 >= AC_MODE_COUNT)) {
                retval = USBRET_BADVALUE;
                goto returnval;
            }
            config.autoclear_mode = param16;
            usbapp_ac_changed = true;
            config_request_save();
            retval = 0;
            break;
        case USBCMD_SETACINTERVAL:
            if ((param16 < AC_1MIN) || (param16 >= AC_INTERVAL_COUNT)) {
                retval = USBRET_BADVALUE;
                goto returnval;
            }
            config.autoclear_interval = param16;
            usbapp_ac_changed = true;
            config_request_save();
            retval = 0;
            break;
        case USBCMD_SETACTHRESHOLD:
            if ((param16 < AC_THRES_HIGH) || (param16 >= AC_THRES_COUNT)) {
                retval = USBRET_BADVALUE;
                goto returnval;
            }
            config.autoclear_threshold = param16;
            usbapp_ac_changed = true;
            config_request_save();
            retval = 0;
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
    txbuf[0] = REPORT_ID_CONTROL;
    txbuf[1] = retval;
    txbuf[2] = cmd;
    txbuf[3] = param;
    txbuf[4] = buffer[13];
    txbuf[5] = buffer[14];
    txbuf[6] = exp_chksum & 0xff;
    txbuf[7] = (exp_chksum >> 8) & 0xff;

    if (!ret)
        return;

    // Stamp getter payloads into reserved response bytes when a GET
    // command completed successfully.
    if ((retval == USBRET_SUCCESS) && tone_getter.pending) {
        tone_getter.pending = false;
        txbuf[8] = (uint8_t)tone_getter.lightness;
        txbuf[9] = (uint8_t)tone_getter.contrast;
    }
    else if ((retval == USBRET_SUCCESS) && mode_getter.pending) {
        mode_getter.pending = false;
        txbuf[8] = mode_getter.mode;
    }
    else if ((retval == USBRET_SUCCESS) && signal_getter.pending) {
        signal_getter.pending = false;
        txbuf[8] = signal_getter.status;
    }
    else if ((retval == USBRET_SUCCESS) && ac_getter.pending) {
        ac_getter.pending = false;
        txbuf[8] = ac_getter.mode;
        txbuf[9] = ac_getter.interval;
        txbuf[10] = ac_getter.threshold;
    }

    tud_hid_report(0, txbuf, CFG_TUD_HID_EP_BUFSIZE);
}

portTASK_FUNCTION(usb_device_task, pvParameters) {
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    
    rxqueue = xQueueCreate(1024, sizeof(char));
    usb_tone_lock = xSemaphoreCreateMutex();
    usb_tone_lock_ready = (usb_tone_lock != NULL);
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    // RTOS forever loop
    while (1) {
        // put this thread to waiting state until there is new events
        tud_task();

        // following code only run if tud_task() process at least 1 event
    }
}
