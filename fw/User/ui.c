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
#include "board.h"
#include "app.h"
#include "ui.h"

static QueueHandle_t btn_queue;

typedef enum {
    BTN1_SHORT_PRESSED,
    BTN1_LONG_PRESSED,
    BTN2_SHORT_PRESSED,
    BTN2_LONG_PRESSED,
    BTN3_SHORT_PRESSED,
    BTN3_LONG_PRESSED
} btn_event_t;

static int mode = 0;

typedef struct {
    update_mode_t id;
    char *name;
} update_mode_item_t;

const update_mode_item_t modes[] = {
    {UM_FAST_MONO_BAYER, "Browsing"},
    {UM_FAST_MONO_BLUE_NOISE, "Watching"},
    {UM_FAST_GREY, "Typing"},
    {UM_AUTO_LUT_NO_DITHER, "Reading"},
};

static int mode_max = sizeof(modes) / sizeof(modes[0]);

#define OSD_WIDTH   256
#define OSD_HEIGHT  128
static uint8_t osd_fb[OSD_WIDTH * OSD_HEIGHT / 8];

void ui_init(void) {
    btn_queue = xQueueCreate(8, sizeof(btn_event_t));
}

static void osd_set_pixel(int x, int y, bool p) {
    if (x >= OSD_WIDTH)
        return;
    if (y >= OSD_HEIGHT)
        return;
    if (x < 0)
        return;
    if (y < 0)
        return;
    int x_byte = x / 8;
    int x_bit = x % 8;
    if (p)
        osd_fb[y * OSD_WIDTH / 8 + x_byte] |= (0x80 >> x_bit);
    else
        osd_fb[y * OSD_WIDTH / 8 + x_byte] &= ~(0x80 >> x_bit);
}

static int osd_draw_char(font_t *font, int x, int y, uint32_t chr, bool fg) {
    uint32_t bw = font->buf_width;
    uint32_t dw = font->disp_width;
    uint32_t h = font->height;
    uint32_t font_byte_per_chr = font->buf_width * font->height / 8;
    const uint8_t *gbuf;
    chr = chr - font->offset;
    if (chr > font->chars)
        return 0;
    gbuf = font->font + chr * font_byte_per_chr;
    for (uint32_t yy = 0; yy < h; yy++) {
        uint8_t byte = 0; // not necessary, to silence warning
        for (uint32_t xx = 0; xx < dw; xx++) {
            if (xx % 8 == 0) {
                byte = gbuf[yy * bw / 8 + xx / 8];
            }
            else {
                byte >>= 1;
            }
            if (byte & 0x1) {
                osd_set_pixel(x + xx, y + yy, fg);
            }
            else {
                osd_set_pixel(x + xx, y + yy, !fg);
            }
        }
    }
    return dw;
}

static void osd_draw_string(font_t *font, int x, int y, char *string, uint32_t maxlen, bool fg) {
    char c;
    uint32_t i = 0;
    uint32_t xx = x;
    while ((i++ < maxlen) && (c = *string++)) {
        xx += osd_draw_char(font, xx, y, c, fg);
    }
}

static void osd_clear(uint8_t c) {
    memset(osd_fb, c, OSD_WIDTH * OSD_HEIGHT / 8);
}

static font_t *load_font(const char *fn) {
    SPIFFS_clearerr(&spiffs_fs);
    spiffs_file f = SPIFFS_open(&spiffs_fs, fn, SPIFFS_O_RDONLY, 0);
    if (SPIFFS_errno(&spiffs_fs) != 0)
        return;
    
    spiffs_stat s;
    SPIFFS_fstat(&spiffs_fs, f, &s);
    uint32_t size = s.size;

    uint8_t *buf = pvPortMalloc(size);
    if (!buf) {
        SPIFFS_close(&spiffs_fs, f);
        return;
    }

    SPIFFS_read(&spiffs_fs, f, buf, size);
    SPIFFS_close(&spiffs_fs, f);

    return (font_t *)buf;
}

static bool is_tmds_active(void) {
    uint8_t val;
    val = adv7611_read_reg(HDMI_I2C_ADDR, 0x04);
    return !!(val & 0x2);
}

static bool is_dp_active(void) {
    return dp_ready;
}

static void restart_fpga(void) {
    bool fpga_up = false;
    int retry = 3;
    while (!fpga_up) {
        fpga_init("fpga.bit");
        int timeout = 10; // 1 sec
        while (timeout) {
            // Wait for PLL to lock
            vTaskDelay(pdMS_TO_TICKS(100));
            uint8_t result = fpga_write_reg8(CSR_ID0, 0x00);
            if (result == 0x35) {
                fpga_up = true;
                break;
            }
            timeout--;
            if (timeout == 0) {
                syslog_printf("FPGA failed to start up (%02x), retrying...", result);
            }
        }
        if (!fpga_up) {
            retry--;
            if (retry == 0) {
                fatal("FPGA failed to start up, giving up.");
            }
        }
    }
}

portTASK_FUNCTION(ui_task, pvParameters) {
    TickType_t osd_timeout = 0;
    bool setmode = false;
    TickType_t autoclear_timeout = 0;
    bool autoclear = false;

    // Load font into memory
    font_t *font_24x40 = load_font("font_24x40.bin");
    font_t *font_32x53 = load_font("font_32x53.bin");

    // First wait link establish
    bool tmds_mode = false;
    while (1) {
        // Check is DP on
        if (is_tmds_active()) {
            ptn3460_powerdown();
            tmds_mode = true;
            break;
        }
        else if (is_dp_active()) {
            adv7611_powerdown();
            tmds_mode = false;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait before next iteration
    }
    restart_fpga();
    power_on_epd();
    caster_init(); // Start refresh
    syslog_printf("FPGA started with status %02x", fpga_write_reg8(CSR_STATUS, 0x00));

    while (1) {
        // Check FPGA lost sync
        if (fpga_write_reg8(CSR_ID0, 0x00) != 0x35) {
            syslog_printf("Lost access to FPGA, attempt to restart...");
            power_off_epd();
            restart_fpga();
            power_on_epd();
            caster_init();
            syslog_printf("FPGA restarted");
        }

        // Check OSD timeout
        if ((osd_timeout != 0) && (((int32_t)xTaskGetTickCount() - (int32_t)osd_timeout) >= 0)) {
            osd_timeout = 0;
            caster_osd_set_enable(false);
        }

        // Update auto clear
        if ((autoclear_timeout != 0) && (((int32_t)xTaskGetTickCount() - (int32_t)autoclear_timeout) >= 0)) {
            caster_redraw(0,0,2400,1800);
            autoclear_timeout = 0; // Pending reset
        }

        if (autoclear && (autoclear_timeout == 0)) {
            autoclear_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(60000);
        }

        if (!autoclear) {
            autoclear_timeout = 0;
        }

        // Detect loss of signal
        if (tmds_mode && (!is_tmds_active())) {
            // Stop and restart when TMDS is detected
            power_off_epd();
            NVIC_SystemReset();
        }

        // Detect signal mode
        // TODO: This should be implemented in FPGA
        if (tmds_mode) {
            uint16_t x, y;
            x = (uint16_t)(adv7611_read_reg(HDMI_I2C_ADDR, 0x07) & 0x1f) << 8;
            x |= adv7611_read_reg(HDMI_I2C_ADDR, 0x08);

            y = (uint16_t)(adv7611_read_reg(HDMI_I2C_ADDR, 0x09) & 0x1f) << 8;
            y |= adv7611_read_reg(HDMI_I2C_ADDR, 0x0a);

            if ((x != config.hact) || (y != config.vact)) {
                fatal("Incorrect input resolution, detected %d x %d.", x, y);
            }
        }

        // Key press logic
        btn_event_t btn_event;
        BaseType_t result = xQueueReceive(btn_queue, &btn_event, pdMS_TO_TICKS(200));
        if (result != pdTRUE)
            continue;
        if (btn_event == BTN1_SHORT_PRESSED) {
            // First key short press
            mode--;
            if (mode < 0) mode = mode_max - 1;
            setmode = true;
        }
        else if (btn_event == BTN2_SHORT_PRESSED) {
            mode++;
            if (mode >= mode_max) mode = 0;
            setmode = true;
        }
        else if ((btn_event == BTN1_LONG_PRESSED) || (btn_event == BTN2_LONG_PRESSED) || (btn_event == BTN3_SHORT_PRESSED)) {
            // Clear screen
            caster_redraw(0,0,2400,1800);
        }
        else if (btn_event == BTN3_LONG_PRESSED) {
            // Toggle auto clearing
            autoclear = !autoclear;
            osd_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
            osd_clear(0xff);
            osd_draw_string(font_24x40, 10, 10, "Auto Clear", 10, false);
            osd_draw_string(font_24x40, 10, 60, autoclear ? "ON" : "OFF", 4, false);
            caster_osd_send_buf(osd_fb);
            caster_osd_set_enable(true);
        }
        if (setmode) {
            caster_setmode(0, 0, config.hact, config.vact, modes[mode].id);
            osd_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
            osd_clear(0xff);
            osd_draw_string(font_24x40, 10, 10, "Mode:", 5, false);
            osd_draw_string(font_24x40, 10, 60, modes[mode].name, 8, false);
            caster_osd_send_buf(osd_fb);
            caster_osd_set_enable(true);
            setmode = false;
        }
    }
}

portTASK_FUNCTION(key_scan_task, pvParameters) {
    while (1) {
        uint32_t scan = button_scan();
        btn_event_t event;
        // No wait, if the ui task doesn't take it, the key press is lost
        if ((scan & BTN_MASK) == BTN_SHORT_PRESSED) {
            event = BTN1_SHORT_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        else if ((scan & BTN_MASK) == BTN_LONG_PRESSED) {
            event = BTN1_LONG_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        if (((scan >> BTN_SHIFT) & BTN_MASK) == BTN_SHORT_PRESSED) {
            event = BTN2_SHORT_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        else if (((scan >> BTN_SHIFT) & BTN_MASK) == BTN_LONG_PRESSED) {
            event = BTN2_LONG_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        if (((scan >> (BTN_SHIFT * 2)) & BTN_MASK) == BTN_SHORT_PRESSED) {
            event = BTN3_SHORT_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        else if (((scan >> (BTN_SHIFT * 2)) & BTN_MASK) == BTN_LONG_PRESSED) {
            event = BTN3_LONG_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
