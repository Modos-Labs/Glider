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
#include <stdio.h>
#include <string.h>
#include "autoclear.h"
#include "ui.h"
#include "osd_font.h"
#include "ui_menu.h"

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

#define OSD_WIDTH           CASTER_OSD_WIDTH
#define OSD_HEIGHT          CASTER_OSD_HEIGHT
#define OSD_STATUS_WIDTH    320
#define OSD_STATUS_HEIGHT   64
#define OSD_BLACK           false
#define OSD_WHITE           true
#define NO_SIGNAL_DELAY_MS  5000
static uint8_t osd_fb[CASTER_OSD_BUF_SIZE];

typedef struct {
    const osd_font_t *small;
    const osd_font_t *item;
    const osd_font_t *large;
} osd_fonts_t;

typedef struct {
    int first_row;
    int category_index;
    ui_menu_depth_t depth;
} osd_menu_render_state_t;

typedef enum {
    SIGNAL_OSD_NONE,
    SIGNAL_OSD_NO_SIGNAL,
    SIGNAL_OSD_UNSUPPORTED,
    SIGNAL_OSD_SLEEPING,
} signal_osd_state_t;

static void apply_input_selection(bool *tmds_mode);

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

static void osd_clear(uint8_t c) {
    memset(osd_fb, c, OSD_WIDTH * OSD_HEIGHT / 8);
}

static void osd_fill_rect(int x, int y, int w, int h, bool color) {
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            osd_set_pixel(xx, yy, color);
        }
    }
}

static void osd_draw_rect(int x, int y, int w, int h, bool color, int thickness) {
    for (int i = 0; i < thickness; i++) {
        osd_fill_rect(x + i, y + i, w - i * 2, 1, color);
        osd_fill_rect(x + i, y + h - 1 - i, w - i * 2, 1, color);
        osd_fill_rect(x + i, y + i, 1, h - i * 2, color);
        osd_fill_rect(x + w - 1 - i, y + i, 1, h - i * 2, color);
    }
}

static const osd_font_t *load_osd_font(const char *fn) {
    SPIFFS_clearerr(&spiffs_fs);
    spiffs_file f = SPIFFS_open(&spiffs_fs, fn, SPIFFS_O_RDONLY, 0);
    if (SPIFFS_errno(&spiffs_fs) != 0)
        return NULL;

    spiffs_stat s;
    SPIFFS_fstat(&spiffs_fs, f, &s);
    uint32_t size = s.size;

    uint8_t *buf = pvPortMalloc(size);
    if (!buf) {
        SPIFFS_close(&spiffs_fs, f);
        return NULL;
    }

    SPIFFS_read(&spiffs_fs, f, buf, size);
    SPIFFS_close(&spiffs_fs, f);

    const osd_font_t *font = osd_font_from_memory(buf, size);
    if (font == NULL) {
        vPortFree(buf);
        return NULL;
    }

    return font;
}

static int osd_draw_prop_char(const osd_font_t *font, int x, int y, uint32_t chr,
        bool color, int max_width) {
    if ((font == NULL) || (max_width <= 0))
        return 0;

    const osd_font_glyph_t *glyph = osd_font_get_glyph(font, chr);
    if (glyph == NULL)
        return 0;

    uint32_t row_words = (font->header.glyph_words - 1) / font->header.height;
    uint32_t draw_width = glyph->width;
    if ((int)draw_width > max_width)
        draw_width = max_width;

    for (uint32_t yy = 0; yy < font->header.height; yy++) {
        const uint32_t *row = glyph->rows + yy * row_words;
        for (uint32_t xx = 0; xx < draw_width; xx++) {
            if (row[xx / 32] & (0x80000000u >> (xx % 32)))
                osd_set_pixel(x + xx, y + yy, color);
        }
    }

    return glyph->width;
}

static void osd_draw_prop_string(const osd_font_t *font, int x, int y, const char *string,
        int max_width, bool color) {
    int xx = x;
    while ((string != NULL) && (*string != '\0') && (xx < x + max_width)) {
        int used = osd_draw_prop_char(font, xx, y, (uint8_t)*string, color,
                x + max_width - xx);
        if (used <= 0)
            used = 4;
        xx += used;
        string++;
    }
}

static void osd_draw_right_string(const osd_font_t *font, int right, int y,
        const char *string, int max_width, bool color) {
    int width = osd_font_text_width(font, string, 64);
    if (width > max_width)
        width = max_width;
    osd_draw_prop_string(font, right - width, y, string, max_width, color);
}

static const osd_font_t *font_or(const osd_font_t *preferred, const osd_font_t *fallback) {
    return preferred ? preferred : fallback;
}

static int mode_index_for(update_mode_t id) {
    for (int i = 0; i < mode_max; i++) {
        if (modes[i].id == id)
            return i;
    }
    return 0;
}

static void draw_status_popup(const osd_fonts_t *fonts, const char *title, const char *value) {
    osd_clear(0xff);
    char textbuf[80];
    snprintf(textbuf, 80, "%s %s", title, value);
    osd_draw_prop_string(font_or(fonts->large, fonts->item), 12, 14, textbuf,
            OSD_STATUS_WIDTH - 24, OSD_BLACK);
    caster_osd_set_window(0, 0, OSD_STATUS_WIDTH, OSD_STATUS_HEIGHT);
    caster_osd_send_buf(osd_fb);
    caster_osd_set_enable(true);
}

static void draw_signal_popup(const osd_fonts_t *fonts, const char *title,
        const char *detail) {
    osd_clear(0xff);
    osd_draw_prop_string(font_or(fonts->large, fonts->item), 12, 12, title,
            OSD_STATUS_WIDTH - 24, OSD_BLACK);
    if ((detail != NULL) && (detail[0] != '\0')) {
        osd_draw_prop_string(font_or(fonts->item, fonts->small), 12, 42, detail,
                OSD_STATUS_WIDTH - 24, OSD_BLACK);
    }
    caster_osd_set_window(0, 0, OSD_STATUS_WIDTH, OSD_STATUS_HEIGHT);
    caster_osd_send_buf(osd_fb);
    caster_osd_set_enable(true);
}

static void menu_render_state_reset(osd_menu_render_state_t *state, const ui_menu_t *menu) {
    state->first_row = 0;
    state->category_index = menu->category_index;
    state->depth = ui_menu_depth(menu);
}

static void draw_menu_row(const osd_fonts_t *fonts, int y, int h, bool selected,
        const char *label, const char *value) {
    bool fg = selected ? OSD_WHITE : OSD_BLACK;
    bool bg = selected ? OSD_BLACK : OSD_WHITE;
    int x = 8;
    int w = OSD_WIDTH - 16;

    osd_fill_rect(x, y, w, h, bg);
    osd_draw_rect(x, y, w, h, OSD_BLACK, selected ? 2 : 1);

    const osd_font_t *item_font = font_or(fonts->item, fonts->small);
    const osd_font_t *value_font = font_or(fonts->small, fonts->item);
    int text_y = y + ((h > 34) ? 8 : 4);
    if (value != NULL && value[0] != '\0') {
        osd_draw_prop_string(item_font, x + 8, text_y, label, w - 110, fg);
        osd_draw_right_string(value_font, x + w - 8, text_y + 2, value, 96, fg);
    }
    else {
        osd_draw_prop_string(item_font, x + 8, text_y, label, w - 16, fg);
    }
}

static void draw_slider_modal(const osd_fonts_t *fonts, const ui_menu_t *menu) {
    int x = 8;
    int y = 34;
    int w = OSD_WIDTH - 16;
    int h = OSD_HEIGHT - y - 22 - 4;
    int rail_x = x + 24;
    int rail_w = w - 48;
    int rail_y = y + 80;
    int count = ui_menu_modal_count(menu);
    int index = ui_menu_modal_index(menu);
    int tick_step = (count > 1) ? rail_w / (count - 1) : 0;
    int knob_x = rail_x + tick_step * index;

    if (tick_step != 0)
        rail_w = tick_step * (count - 1);

    osd_fill_rect(x, y, w, h, OSD_WHITE);
    osd_draw_rect(x, y, w, h, OSD_BLACK, 3);

    osd_draw_prop_string(font_or(fonts->item, fonts->small), x + 8, y + 10,
            ui_menu_selected_label(menu), w - 16, OSD_BLACK);
    osd_draw_prop_string(font_or(fonts->large, fonts->item), OSD_WIDTH / 2 - 10,
            y + 40, ui_menu_modal_value_label(menu), w - 24, OSD_BLACK);

    osd_fill_rect(rail_x, rail_y - 2, rail_w, 4, OSD_BLACK);
    for (int i = 0; i < count; i++) {
        int tx = rail_x + tick_step * i;
        osd_fill_rect(tx - 1, rail_y - 7, 2, 14, OSD_BLACK);
    }
    osd_fill_rect(knob_x - 6, rail_y - 10, 12, 20, OSD_BLACK);

    osd_draw_prop_string(font_or(fonts->small, fonts->item), rail_x - 8, rail_y + 18,
            "-3", 36, OSD_BLACK);
    osd_draw_right_string(font_or(fonts->small, fonts->item), rail_x + rail_w + 8,
            rail_y + 18, "3", 36, OSD_BLACK);
}

static void draw_list_modal(const osd_fonts_t *fonts, const ui_menu_t *menu,
        int first, int visible) {
    int x = 8;
    int y = 34;
    int w = OSD_WIDTH - 16;
    int h = OSD_HEIGHT - y - 22 - 4;
    int row_top = y + 30;
    int row_h = 26;
    int count = ui_menu_row_count(menu);

    osd_fill_rect(x, y, w, h, OSD_WHITE);
    osd_draw_rect(x, y, w, h, OSD_BLACK, 3);

    osd_draw_prop_string(font_or(fonts->item, fonts->small), x + 8, y + 8,
            ui_menu_selected_label(menu), w - 16, OSD_BLACK);

    if (visible > (h - 34) / row_h)
        visible = (h - 34) / row_h;
    if (visible < 1)
        visible = 1;

    for (int row = first; (row < count) && (row < first + visible); row++) {
        bool selected = ui_menu_row_selected(menu, row);
        bool fg = selected ? OSD_WHITE : OSD_BLACK;
        bool bg = selected ? OSD_BLACK : OSD_WHITE;
        int yy = row_top + (row - first) * row_h;
        osd_fill_rect(x + 8, yy, w - 16, row_h - 3, bg);
        osd_draw_prop_string(font_or(fonts->small, fonts->item), x + 11, yy + 2,
                ui_menu_row_label(menu, row), w - 22, fg);
    }
}

static int menu_selected_row(const ui_menu_t *menu) {
    int rows = ui_menu_row_count(menu);
    for (int i = 0; i < rows; i++) {
        if (ui_menu_row_selected(menu, i))
            return i;
    }
    return 0;
}

static void render_settings_menu(const osd_fonts_t *fonts, const ui_menu_t *menu,
        osd_menu_render_state_t *render_state) {
    osd_clear(0xff);

    if ((render_state->category_index != menu->category_index) ||
            (render_state->depth != ui_menu_depth(menu))) {
        menu_render_state_reset(render_state, menu);
    }

    osd_fill_rect(0, 0, OSD_WIDTH, 28, OSD_BLACK);
    osd_draw_prop_string(font_or(fonts->item, fonts->small), 8, 3, "Settings",
            90, OSD_WHITE);
    osd_draw_prop_string(font_or(fonts->small, fonts->item), 110, 7,
            (ui_menu_depth(menu) == UI_MENU_DEPTH_CATEGORIES) ? "Top Level" :
            ui_menu_category_label(menu), OSD_WIDTH - 118, OSD_WHITE);

    int top = 34;
    int bottom = OSD_HEIGHT - 22;
    int row_h = (ui_menu_depth(menu) == UI_MENU_DEPTH_CATEGORIES) ? 46 : 36;
    int visible = (bottom - top) / row_h;
    int count = ui_menu_row_count(menu);
    int selected = menu_selected_row(menu);
    int viewport_visible = visible;
    if ((ui_menu_depth(menu) == UI_MENU_DEPTH_MODAL) && !ui_menu_modal_is_scalar(menu))
        viewport_visible = (OSD_HEIGHT - 34 - 22 - 4 - 34) / 26;
    int first = ui_menu_viewport_first(render_state->first_row, selected, count, viewport_visible);
    render_state->first_row = first;

    if (visible < 1)
        visible = 1;
    if (viewport_visible < 1)
        viewport_visible = 1;

    if (ui_menu_depth(menu) == UI_MENU_DEPTH_MODAL) {
        if (ui_menu_modal_is_scalar(menu))
            draw_slider_modal(fonts, menu);
        else
            draw_list_modal(fonts, menu, first, viewport_visible);
    }
    else {
        for (int row = first; (row < count) && (row < first + visible); row++) {
            draw_menu_row(fonts, top + (row - first) * row_h, row_h - 4,
                ui_menu_row_selected(menu, row),
                ui_menu_row_label(menu, row),
                ui_menu_row_value(menu, row));
        }
    }

    osd_draw_rect(0, OSD_HEIGHT - 20, OSD_WIDTH, 20, OSD_BLACK, 1);
    const char *hint;
    if ((ui_menu_depth(menu) == UI_MENU_DEPTH_MODAL) && ui_menu_modal_is_scalar(menu))
        hint = "B1/B2 Adj  B3 OK  L3 Back";
    else if (ui_menu_depth(menu) == UI_MENU_DEPTH_MODAL)
        hint = "B1/B2 Up/Dn  B3 OK  L3 Back";
    else
        hint = "B1/B2 Up/Dn  B3 Ent  L3 Back";
    osd_draw_prop_string(font_or(fonts->small, fonts->item), 5, OSD_HEIGHT - 20,
            hint, OSD_WIDTH - 48, OSD_BLACK);

    char page[12];
    snprintf(page, sizeof(page), "%d/%d", selected + 1, count);
    osd_draw_right_string(font_or(fonts->small, fonts->item), OSD_WIDTH - 5,
            OSD_HEIGHT - 20, page, 42, OSD_BLACK);

    caster_osd_set_window(0, config.tcon_vact -
        OSD_HEIGHT * (config.osd_scale_2x ? 2u : 1u), OSD_WIDTH, OSD_HEIGHT);
    caster_osd_send_buf(osd_fb);
    caster_osd_set_enable(true);
}

static int binding_index_for_event(btn_event_t event) {
    switch (event) {
    case BTN1_SHORT_PRESSED:
        return 0;
    case BTN2_SHORT_PRESSED:
        return 1;
    case BTN3_SHORT_PRESSED:
        return 2;
    case BTN1_LONG_PRESSED:
        return 3;
    case BTN2_LONG_PRESSED:
        return 4;
    case BTN3_LONG_PRESSED:
        return 5;
    default:
        return -1;
    }
}

static const char *input_label(uint8_t input) {
    switch (input) {
    case INPUT_SEL_TMDS:
        return "TMDS";
    case INPUT_SEL_DP:
        return "DP";
    case INPUT_SEL_AUTO:
    default:
        return "Auto";
    }
}

static const char *autoclear_mode_label(void) {
    switch (config.autoclear_mode) {
    case AC_ADAPT:
        return "Adaptive";
    case AC_FIXED:
        return "Fixed";
    case AC_OFF:
    default:
        return "OFF";
    }
}

static void apply_display_mode(update_mode_t id, const osd_fonts_t *fonts,
        TickType_t *osd_timeout) {
    mode = mode_index_for(id);
    config.update_mode = modes[mode].id;
    caster_setmode(0, 0, config.hact, config.vact, modes[mode].id);
    config_save();

    draw_status_popup(fonts, "Mode", modes[mode].name);
    *osd_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
}

static bool menu_config_changed(const config_t *previous) {
    return (previous->input_sel != config.input_sel) ||
            (previous->update_mode != config.update_mode) ||
            (previous->lightness != config.lightness) ||
            (previous->contrast != config.contrast) ||
            (previous->autoclear_mode != config.autoclear_mode) ||
            (previous->autoclear_interval != config.autoclear_interval) ||
            (previous->autoclear_threshold != config.autoclear_threshold) ||
            (previous->osd_scale_2x != config.osd_scale_2x) ||
            (memcmp(previous->button_actions, config.button_actions,
                    sizeof(config.button_actions)) != 0);
}

static void preview_tone_modal(const ui_menu_t *menu) {
    int lightness = config.lightness;
    int contrast = config.contrast;
    int target = ui_menu_modal_tone_target(menu);

    if (target == 0)
        return;

    if (target == 1)
        lightness = ui_menu_modal_preview_value(menu);
    else if (target == 2)
        contrast = ui_menu_modal_preview_value(menu);

    caster_set_tone(lightness, contrast);
}

static void reset_autoclear_state(TickType_t *autoclear_timeout,
        uint32_t *autoclear_damage_counter, uint32_t *autoclear_damage_last) {
    if (autoclear_timeout != NULL)
        *autoclear_timeout = 0;
    autoclear_reset(autoclear_damage_counter, autoclear_damage_last);
}

static void execute_button_action(button_action_t action, const osd_fonts_t *fonts,
        ui_menu_t *menu, bool *menu_open, bool *autoclear,
        TickType_t *osd_timeout, TickType_t *autoclear_timeout,
        uint32_t *autoclear_damage_counter, uint32_t *autoclear_damage_last,
        osd_menu_render_state_t *render_state, int *autoclear_previous_mode,
        bool *tmds_mode) {
    switch (action) {
    case ACT_PREV_MODE:
        mode--;
        if (mode < 0)
            mode = mode_max - 1;
        apply_display_mode(modes[mode].id, fonts, osd_timeout);
        break;
    case ACT_NEXT_MODE:
        mode++;
        if (mode >= mode_max)
            mode = 0;
        apply_display_mode(modes[mode].id, fonts, osd_timeout);
        break;
    case ACT_MODE_BROWSING:
        apply_display_mode(UM_FAST_MONO_BAYER, fonts, osd_timeout);
        break;
    case ACT_MODE_WATCHING:
        apply_display_mode(UM_FAST_MONO_BLUE_NOISE, fonts, osd_timeout);
        break;
    case ACT_MODE_TYPING:
        apply_display_mode(UM_FAST_GREY, fonts, osd_timeout);
        break;
    case ACT_MODE_READING:
        apply_display_mode(UM_AUTO_LUT_NO_DITHER, fonts, osd_timeout);
        break;
    case ACT_CLEAR:
        caster_redraw(0, 0, config.hact, config.vact);
        reset_autoclear_state(autoclear_timeout, autoclear_damage_counter,
                autoclear_damage_last);
        break;
    case ACT_TOGGLE_AC:
        config.autoclear_mode = autoclear_toggle_mode(config.autoclear_mode,
                autoclear_previous_mode);
        *autoclear = config.autoclear_mode != AC_OFF;
        reset_autoclear_state(autoclear_timeout, autoclear_damage_counter,
                autoclear_damage_last);
        config_save();
        draw_status_popup(fonts, "Auto Clear", autoclear_mode_label());
        *osd_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
        break;
    case ACT_OPEN_SETTINGS:
        ui_menu_init(menu, &config);
        menu_render_state_reset(render_state, menu);
        *menu_open = true;
        *osd_timeout = 0;
        render_settings_menu(fonts, menu, render_state);
        break;
    case ACT_POFF:
        draw_status_popup(fonts, "Power", "Off");
        caster_redraw_blank();
        sleep_ms(200);
        caster_redraw(0, 0, config.hact, config.vact);
        sleep_ms(600);
        power_off();
        break;
    case ACT_INPUT_AUTO:
        config.input_sel = INPUT_SEL_AUTO;
        config_save();
        apply_input_selection(tmds_mode);
        draw_status_popup(fonts, "Input", input_label(config.input_sel));
        *osd_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
        break;
    case ACT_INPUT_TMDS:
        config.input_sel = INPUT_SEL_TMDS;
        config_save();
        apply_input_selection(tmds_mode);
        draw_status_popup(fonts, "Input", input_label(config.input_sel));
        *osd_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
        break;
    case ACT_INPUT_DP:
        config.input_sel = INPUT_SEL_DP;
        config_save();
        apply_input_selection(tmds_mode);
        draw_status_popup(fonts, "Input", input_label(config.input_sel));
        *osd_timeout = xTaskGetTickCount() + pdMS_TO_TICKS(2000);
        break;
    default:
        break;
    }
}

static bool is_tmds_active(void) {
    uint8_t val;
    val = adv7611_read_reg(HDMI_I2C_ADDR, 0x04);
    return !!(val & 0x2);
}

static bool is_dp_active(void) {
    return dp_ready;
}

static bool is_dp_video_active(void) {
    return gpio_get(DP_ACTIVE) == GPIO_PIN_SET;
}

static bool is_video_active(bool tmds_mode) {
    return tmds_mode ? is_tmds_active() : is_dp_video_active();
}

static bool is_selected_video_active(bool tmds_mode) {
    if (config.input_sel == INPUT_SEL_AUTO)
        return is_tmds_active() || is_dp_video_active();
    return is_video_active(tmds_mode);
}

static void resume_video_frontends(void);

static void apply_input_selection(bool *tmds_mode) {
    switch (config.input_sel) {
    case INPUT_SEL_TMDS:
        *tmds_mode = true;
        syslog_print("Requesting TMDS input");
        adv7611_early_init();
        adv7611_init();
        ptn3460_powerdown();
        caster_input_request_tmds();
        break;
    case INPUT_SEL_DP:
        *tmds_mode = false;
        syslog_print("Requesting DP input");
        ptn3460_init();
        usbpd_resume_displayport();
        adv7611_powerdown();
        caster_input_request_dp();
        break;
    case INPUT_SEL_AUTO:
    default:
        if (is_tmds_active())
            *tmds_mode = true;
        else if (is_dp_active())
            *tmds_mode = false;
        syslog_print("Requesting auto input");
        resume_video_frontends();
        caster_input_request_auto();
        break;
    }
}

static void restart_fpga(void) {
    fpga_init(config.bitstream);
    syslog_printf("Waiting for FPGA CSR access");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
        uint8_t result = fpga_write_reg8(CSR_ID0, 0x00);
        if (result == 0x35) {
            break;
        }
    }
    syslog_printf("FPGA up");
}

// How long to wait at startup for a video input to become live before loading
// the bitstream. The decoder does not report lock immediately after
// adv7611_init() -- measured around 1.1s against a host that was already
// running, and far longer if the host is still booting. Generous on purpose:
// with a live source the wait exits as soon as it sees signal, so this only
// elapses when there is genuinely nothing attached.
#define VIDEO_INPUT_WAIT_MS     30000
#define VIDEO_INPUT_POLL_MS     100

// Minimum interval between pipeline reloads after video is lost. The reload
// path must NOT wait for an input -- video has just gone away, so there is
// nothing to wait for and waiting delays the no-signal state by the full
// timeout. But it does need a floor: without one, lose-reload-lose cycles as
// fast as the bitstream loads, which shows up as the panel flashing.
#define VIDEO_RELOAD_MIN_MS     5000

// Bring up the video frontends and wait for an input to go live. This must run
// BEFORE restart_fpga(): initialising the decoder resets its pixel clock and
// reasserts HPD, and doing that after the gateware has started locking prevents
// TMDS from ever coming up.
//
static void prepare_video_input(bool *tmds_mode, bool wait_for_input) {
    switch (config.input_sel) {
    case INPUT_SEL_TMDS:
        *tmds_mode = true;
        adv7611_early_init();
        adv7611_init();
        ptn3460_powerdown();
        break;
    case INPUT_SEL_DP:
        *tmds_mode = false;
        ptn3460_init();
        usbpd_resume_displayport();
        adv7611_powerdown();
        break;
    case INPUT_SEL_AUTO:
    default:
        resume_video_frontends();
        break;
    }

    if (!wait_for_input)
        return;

    for (int i = 0; i < (VIDEO_INPUT_WAIT_MS / VIDEO_INPUT_POLL_MS); i++) {
        if (is_tmds_active()) {
            if (config.input_sel == INPUT_SEL_AUTO)
                *tmds_mode = true;
            return;
        }
        if (is_dp_active()) {
            if (config.input_sel == INPUT_SEL_AUTO)
                *tmds_mode = false;
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(VIDEO_INPUT_POLL_MS));
    }
    syslog_print("No live video input; continuing");
}

// The half that has to run after restart_fpga(), because CSR_INPUT_CTRL needs
// the FPGA to be up.
static void commit_input_selection(void) {
    switch (config.input_sel) {
    case INPUT_SEL_TMDS:
        caster_input_request_tmds();
        break;
    case INPUT_SEL_DP:
        caster_input_request_dp();
        break;
    case INPUT_SEL_AUTO:
    default:
        caster_input_request_auto();
        break;
    }
}

static void start_display_pipeline_ex(bool *tmds_mode, const osd_fonts_t *fonts,
        signal_osd_state_t *signal_osd_state, TickType_t *no_signal_deadline,
        bool wait_for_input) {
    prepare_video_input(tmds_mode, wait_for_input);
    restart_fpga();
    power_on_epd();
    caster_init();
    commit_input_selection();
    caster_osd_set_enable(false);
    *signal_osd_state = SIGNAL_OSD_NONE;
    *no_signal_deadline = xTaskGetTickCount() +
            pdMS_TO_TICKS(NO_SIGNAL_DELAY_MS);
    syslog_printf("FPGA started with status %02x", fpga_write_reg8(CSR_STATUS, 0x00));
}

static void resume_video_frontends(void) {
    adv7611_early_init();
    adv7611_init();
    ptn3460_init();
    usbpd_resume_displayport();
}

static void update_input_tracking(uint8_t input_status, bool *tmds_mode,
        bool *live_was_selected) {
    if (input_status & INPUT_STATUS_TMDS)
        *tmds_mode = true;
    else if (input_status & INPUT_STATUS_DP)
        *tmds_mode = false;
    *live_was_selected = !!(input_status & INPUT_STATUS_LIVE);
}

static void update_signal_osd(const osd_fonts_t *fonts, uint8_t input_status,
        bool menu_open, TickType_t osd_timeout,
        signal_osd_state_t *signal_osd_state, TickType_t *no_signal_deadline) {
    if (menu_open || (osd_timeout != 0))
        return;

    if (input_status & INPUT_STATUS_LIVE) {
        if (*signal_osd_state != SIGNAL_OSD_NONE)
            caster_osd_set_enable(false);
        *signal_osd_state = SIGNAL_OSD_NONE;
        *no_signal_deadline = 0;
        return;
    }

    if ((input_status & INPUT_STATUS_STABLE) &&
            !(input_status & INPUT_STATUS_SUPPORTED)) {
        if (*signal_osd_state != SIGNAL_OSD_UNSUPPORTED) {
            uint16_t hact;
            uint16_t vact;
            char detail[32];
            caster_input_get_measured(&hact, &vact, NULL, NULL);
            if ((hact != 0) && (vact != 0))
                snprintf(detail, sizeof(detail), "%u x %u", hact, vact);
            else
                detail[0] = '\0';
            draw_signal_popup(fonts, "Unsupported Input", detail);
            *signal_osd_state = SIGNAL_OSD_UNSUPPORTED;
        }
        *no_signal_deadline = 0;
        return;
    }

    if ((*no_signal_deadline != 0) &&
            (((int32_t)xTaskGetTickCount() - (int32_t)*no_signal_deadline) < 0)) {
        if (*signal_osd_state != SIGNAL_OSD_NONE)
            caster_osd_set_enable(false);
        *signal_osd_state = SIGNAL_OSD_NONE;
        return;
    }
    *no_signal_deadline = 0;

    if (*signal_osd_state != SIGNAL_OSD_NO_SIGNAL) {
        draw_signal_popup(fonts, "No Signal", NULL);
        *signal_osd_state = SIGNAL_OSD_NO_SIGNAL;
    }
}

static void log_input_status(uint8_t input_status) {
    uint16_t hact;
    uint16_t vact;
    uint16_t htotal;
    uint16_t vtotal;

    caster_input_get_measured(&hact, &vact, &htotal, &vtotal);
    syslog_printf("Input status %02x, measured %u x %u, total %u x %u",
            input_status, hact, vact, htotal, vtotal);
}

static void start_display_pipeline(bool *tmds_mode, const osd_fonts_t *fonts,
        signal_osd_state_t *signal_osd_state, TickType_t *no_signal_deadline) {
    start_display_pipeline_ex(tmds_mode, fonts, signal_osd_state,
            no_signal_deadline, true);
}

static void reload_to_internal_source(bool *tmds_mode, const osd_fonts_t *fonts,
        signal_osd_state_t *signal_osd_state, TickType_t *no_signal_deadline,
        const char *reason) {
    syslog_print(reason);
    power_off_epd();
    start_display_pipeline_ex(tmds_mode, fonts, signal_osd_state,
            no_signal_deadline, false);
}

static void recover_from_live_loss(bool *tmds_mode, const osd_fonts_t *fonts,
        signal_osd_state_t *signal_osd_state, TickType_t *no_signal_deadline) {
    reload_to_internal_source(tmds_mode, fonts, signal_osd_state,
            no_signal_deadline,
            "Live video lost; reloading FPGA into internal source");
    draw_signal_popup(fonts, "Sleeping", NULL);
    *signal_osd_state = SIGNAL_OSD_SLEEPING;
    *no_signal_deadline = 0;
    caster_redraw(0, 0, config.hact, config.vact);
    sleep_ms(600);
    power_suspend(POWER_SUSPEND_VIDEO_LOSS);
}

static uint32_t wait_for_wake_source(bool *usbpd_wake_armed,
        TickType_t usbpd_wake_arm_time, bool tmds_mode) {
    btn_event_t btn_event;
    power_suspend_reason_t reason = power_get_current_suspend_reason();

    if ((reason == POWER_SUSPEND_VIDEO_LOSS) && is_selected_video_active(tmds_mode))
        return POWER_WAKE_INPUT;

    if (usbapp_take_resume_event())
        return POWER_WAKE_USB;

    if (xQueueReceive(btn_queue, &btn_event, pdMS_TO_TICKS(200)) == pdTRUE)
        return POWER_WAKE_BUTTON;

    if (usbapp_take_resume_event())
        return POWER_WAKE_USB;

    if (!*usbpd_wake_armed &&
            (((int32_t)xTaskGetTickCount() - (int32_t)usbpd_wake_arm_time) >= 0)) {
        usbpd_arm_wake();
        *usbpd_wake_armed = true;
    }

    if (*usbpd_wake_armed && usbpd_take_wake_event())
        return POWER_WAKE_USB_PD;
    return POWER_WAKE_NONE;
}

portTASK_FUNCTION(ui_task, pvParameters) {
    TickType_t osd_timeout = 0;
    TickType_t autoclear_timeout = 0;
    uint32_t autoclear_damage_counter = 0;
    uint32_t autoclear_damage_last = 0;
    int autoclear_previous_mode = (config.autoclear_mode == AC_ADAPT ||
            config.autoclear_mode == AC_FIXED) ? config.autoclear_mode : AC_ADAPT;
    bool autoclear = config.autoclear_mode != AC_OFF;
    bool menu_open = false;
    bool suspend_wait_initialized = false;
    bool usbpd_wake_armed = false;
    bool live_was_selected = false;
    uint8_t last_logged_input_status = 0xff;
    TickType_t no_signal_deadline = 0;
    // When the pipeline was last reloaded after video loss, for rate limiting.
    TickType_t last_reload_time = 0;
    TickType_t usbpd_wake_arm_time = 0;
    ui_menu_t menu;
    osd_menu_render_state_t menu_render_state = {0};
    signal_osd_state_t signal_osd_state = SIGNAL_OSD_NONE;

    osd_fonts_t fonts = {
        .small = load_osd_font("fonts/font_quicksand_16.bin"),
        .item = load_osd_font("fonts/font_quicksand_20.bin"),
        .large = load_osd_font("fonts/font_quicksand_28.bin"),
    };

    mode = mode_index_for((update_mode_t)config.update_mode);

    bool tmds_mode = false;
    start_display_pipeline(&tmds_mode, &fonts, &signal_osd_state,
            &no_signal_deadline);

    while (1) {
        if (power_is_suspended()) {
            if (!suspend_wait_initialized) {
                usbpd_disarm_wake();
                usbpd_wake_armed = false;
                usbpd_wake_arm_time = xTaskGetTickCount() + pdMS_TO_TICKS(1000);
                suspend_wait_initialized = true;
            }

            uint32_t wake_sources = wait_for_wake_source(&usbpd_wake_armed,
                    usbpd_wake_arm_time, tmds_mode);
            if (!power_request_resume(wake_sources))
                continue;

            syslog_printf("Waking system: 0x%02x", (unsigned)wake_sources);
            usbpd_disarm_wake();
            fpga_resume();
            if (power_get_last_suspend_reason() != POWER_SUSPEND_VIDEO_LOSS)
                resume_video_frontends();
            start_display_pipeline(&tmds_mode, &fonts, &signal_osd_state,
                    &no_signal_deadline);
            power_resume_complete();

            menu_open = false;
            suspend_wait_initialized = false;
            usbpd_wake_armed = false;
            live_was_selected = false;
            osd_timeout = 0;
            autoclear_timeout = 0;
            autoclear = config.autoclear_mode != AC_OFF;
            mode = mode_index_for((update_mode_t)config.update_mode);
            reset_autoclear_state(&autoclear_timeout, &autoclear_damage_counter,
                    &autoclear_damage_last);
            menu_render_state.first_row = 0;
            menu_render_state.category_index = 0;
            menu_render_state.depth = UI_MENU_DEPTH_CATEGORIES;
            continue;
        }

        (void)usbapp_take_resume_event();
        if (usbapp_take_suspend_event()) {
            syslog_print("USB bus suspended; suspending");
            power_suspend(POWER_SUSPEND_USB);
            continue;
        }

        // Check OSD timeout
        if ((osd_timeout != 0) && (((int32_t)xTaskGetTickCount() - (int32_t)osd_timeout) >= 0)) {
            osd_timeout = 0;
            caster_osd_set_enable(false);
        }

        // Update auto clear
        bool autoclear_trigger = false;
        if (autoclear && (config.autoclear_mode == AC_ADAPT)) {
            autoclear_trigger = autoclear_adaptive_update(
                    &autoclear_damage_counter, &autoclear_damage_last,
                    caster_get_damage_counter(), config.autoclear_threshold,
                    config.hact, config.vact);
        }

        if ((autoclear_timeout != 0) && (((int32_t)xTaskGetTickCount() - (int32_t)autoclear_timeout) >= 0)) {
            autoclear_trigger = true;
            autoclear_timeout = 0; // Pending reset
        }

        if (autoclear_trigger) {
            caster_redraw(0, 0, config.hact, config.vact);
            reset_autoclear_state(&autoclear_timeout, &autoclear_damage_counter,
                    &autoclear_damage_last);
        }

        if (autoclear && (config.autoclear_mode == AC_FIXED) &&
                (autoclear_timeout == 0)) {
            autoclear_timeout = xTaskGetTickCount() +
                    pdMS_TO_TICKS(autoclear_fixed_interval_ms(
                            config.autoclear_interval));
        }

        if (!autoclear) {
            reset_autoclear_state(&autoclear_timeout, &autoclear_damage_counter,
                    &autoclear_damage_last);
        }

        // Detect loss of signal / lost access to FPGA. If live video was
        // selected, a stopped external clock can also stop CSR access, so use
        // the frontend signal check before touching FPGA registers.
        if (live_was_selected && !is_video_active(tmds_mode)) {
            // Rate limit the reload. The wait inside prepare_video_input()
            // used to provide this by accident; the reload path no longer
            // waits, so enforce a floor explicitly or a flapping input
            // reloads the bitstream continuously.
            TickType_t now = xTaskGetTickCount();
            if ((last_reload_time != 0) &&
                    (((int32_t)now - (int32_t)last_reload_time) <
                     (int32_t)pdMS_TO_TICKS(VIDEO_RELOAD_MIN_MS))) {
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }
            last_reload_time = now;
            recover_from_live_loss(&tmds_mode, &fonts, &signal_osd_state,
                    &no_signal_deadline);
            live_was_selected = false;
            continue;
        }
        if (fpga_write_reg8(CSR_ID0, 0x00) != 0x35) {
            syslog_print("FPGA access lost; resetting");
            power_off_epd();
            NVIC_SystemReset();
        }

        uint8_t input_status = caster_input_status();
        if (input_status != last_logged_input_status) {
            log_input_status(input_status);
            last_logged_input_status = input_status;
        }
        update_input_tracking(input_status, &tmds_mode, &live_was_selected);
        if (input_status & INPUT_STATUS_LOST) {
            if (is_selected_video_active(tmds_mode)) {
                reload_to_internal_source(&tmds_mode, &fonts, &signal_osd_state,
                        &no_signal_deadline,
                        "FPGA source lost while frontend is active; retrying internal source");
            }
            else {
                recover_from_live_loss(&tmds_mode, &fonts, &signal_osd_state,
                        &no_signal_deadline);
            }
            live_was_selected = false;
            last_logged_input_status = 0xff;
            continue;
        }
        update_signal_osd(&fonts, input_status, menu_open, osd_timeout,
                &signal_osd_state, &no_signal_deadline);

        // Detect signal mode
        // TODO: This should be implemented in FPGA
#if 0
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
#endif

        // Key press logic
        btn_event_t btn_event;
        BaseType_t result = xQueueReceive(btn_queue, &btn_event, pdMS_TO_TICKS(200));
        if (result != pdTRUE)
            continue;

        if (menu_open) {
            config_t previous_config = config;
            update_mode_t previous_mode = (update_mode_t)config.update_mode;
            ui_menu_event_t menu_event = UI_MENU_EVENT_ENTER;
            bool close_menu = false;
            bool was_tone_modal = ui_menu_modal_is_tone(&menu);

            if (btn_event == BTN1_SHORT_PRESSED)
                menu_event = UI_MENU_EVENT_PREV;
            else if (btn_event == BTN2_SHORT_PRESSED)
                menu_event = UI_MENU_EVENT_NEXT;
            else if (btn_event == BTN3_SHORT_PRESSED)
                menu_event = UI_MENU_EVENT_ENTER;
            else if (btn_event == BTN3_LONG_PRESSED) {
                if (ui_menu_depth(&menu) == UI_MENU_DEPTH_CATEGORIES)
                    close_menu = true;
                else
                    menu_event = UI_MENU_EVENT_BACK;
            }
            else {
                render_settings_menu(&fonts, &menu, &menu_render_state);
                continue;
            }

            if (close_menu) {
                menu_open = false;
                caster_osd_set_enable(false);
            }
            else {
                ui_menu_handle(&menu, menu_event);
                if (was_tone_modal && (menu_event == UI_MENU_EVENT_BACK)) {
                    caster_set_tone(config.lightness, config.contrast);
                }
                else if (ui_menu_modal_is_tone(&menu) &&
                        ((menu_event == UI_MENU_EVENT_PREV) ||
                         (menu_event == UI_MENU_EVENT_NEXT))) {
                    preview_tone_modal(&menu);
                }
                if (menu_config_changed(&previous_config)) {
                    if (previous_config.autoclear_mode != config.autoclear_mode) {
                        if ((config.autoclear_mode == AC_ADAPT) ||
                                (config.autoclear_mode == AC_FIXED)) {
                            autoclear_previous_mode = config.autoclear_mode;
                        }
                        else if ((previous_config.autoclear_mode == AC_ADAPT) ||
                                (previous_config.autoclear_mode == AC_FIXED)) {
                            autoclear_previous_mode = previous_config.autoclear_mode;
                        }
                    }
                    config_save();
                    autoclear = config.autoclear_mode != AC_OFF;
                    if (previous_config.input_sel != config.input_sel) {
                        apply_input_selection(&tmds_mode);
                        live_was_selected = false;
                    }
                    if ((previous_config.lightness != config.lightness) ||
                            (previous_config.contrast != config.contrast)) {
                        caster_set_tone(config.lightness, config.contrast);
                    }
                    if ((previous_config.autoclear_mode != config.autoclear_mode) ||
                            (previous_config.autoclear_interval != config.autoclear_interval) ||
                            (previous_config.autoclear_threshold != config.autoclear_threshold)) {
                        reset_autoclear_state(&autoclear_timeout,
                                &autoclear_damage_counter,
                                &autoclear_damage_last);
                    }
                    if (previous_mode != (update_mode_t)config.update_mode) {
                        mode = mode_index_for((update_mode_t)config.update_mode);
                        caster_setmode(0, 0, config.hact, config.vact,
                                modes[mode].id);
                        reset_autoclear_state(&autoclear_timeout,
                                &autoclear_damage_counter,
                                &autoclear_damage_last);
                    }
                }
                render_settings_menu(&fonts, &menu, &menu_render_state);
            }
            osd_timeout = 0;
            continue;
        }

        int binding = binding_index_for_event(btn_event);
        if ((binding >= 0) && (binding < CONFIG_BUTTON_BINDING_COUNT)) {
            execute_button_action((button_action_t)config.button_actions[binding],
                    &fonts, &menu, &menu_open, &autoclear, &osd_timeout,
                    &autoclear_timeout, &autoclear_damage_counter,
                    &autoclear_damage_last, &menu_render_state,
                    &autoclear_previous_mode, &tmds_mode);
        }
    }
}

portTASK_FUNCTION(key_scan_task, pvParameters) {
    while (1) {
        uint32_t scan = button_scan();
        btn_event_t event;
        // No wait, if the ui task doesn't take it, the key press is lost
        if ((scan & BTN_MASK) == BTN_SHORT_PRESSED) {
            event = BTN3_SHORT_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        else if ((scan & BTN_MASK) == BTN_LONG_PRESSED) {
            event = BTN3_LONG_PRESSED;
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
            event = BTN1_SHORT_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        else if (((scan >> (BTN_SHIFT * 2)) & BTN_MASK) == BTN_LONG_PRESSED) {
            event = BTN1_LONG_PRESSED;
            xQueueSend(btn_queue, &event, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
