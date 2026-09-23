#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "ui_menu.h"

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        printf("ASSERT_TRUE failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)

#define ASSERT_EQ(expected, actual) do { \
    long long exp__ = (long long)(expected); \
    long long act__ = (long long)(actual); \
    if (exp__ != act__) { \
        printf("ASSERT_EQ failed at %s:%d: expected %lld got %lld\n", \
                __FILE__, __LINE__, exp__, act__); \
        return 1; \
    } \
} while (0)

static int test_config_defaults_match_button_design(void) {
    config_init();

    ASSERT_EQ(CONFIG_SCHEMA_VERSION, config.schema_version);
    ASSERT_EQ(ACT_NEXT_MODE, config.button_actions[0]);
    ASSERT_EQ(ACT_PREV_MODE, config.button_actions[1]);
    ASSERT_EQ(ACT_OPEN_SETTINGS, config.button_actions[2]);
    ASSERT_EQ(ACT_POFF, config.button_actions[3]);
    ASSERT_EQ(ACT_CLEAR, config.button_actions[4]);
    ASSERT_EQ(ACT_TOGGLE_AC, config.button_actions[5]);
    ASSERT_EQ(AC_OFF, config.autoclear_mode);
    ASSERT_EQ(AC_5MIN, config.autoclear_interval);
    ASSERT_EQ(AC_THRES_MED, config.autoclear_threshold);
    ASSERT_EQ(0, config.osd_scale_2x);

    return 0;
}

static int test_legacy_config_validation_repairs_appended_fields(void) {
    config_init();

    config.schema_version = 0;
    for (int i = 0; i < CONFIG_BUTTON_BINDING_COUNT; i++)
        config.button_actions[i] = -99;
    config.update_mode = -1;
    config.lightness = 99;
    config.contrast = -99;
    config.reserved_tone = 99;
    config.autoclear_mode = 99;
    config.autoclear_interval = 99;
    config.autoclear_threshold = 99;
    config.osd_scale_2x = 99;

    config_validate_loaded(offsetof(config_t, schema_version));

    ASSERT_EQ(CONFIG_SCHEMA_VERSION, config.schema_version);
    ASSERT_EQ(ACT_NEXT_MODE, config.button_actions[0]);
    ASSERT_EQ(ACT_PREV_MODE, config.button_actions[1]);
    ASSERT_EQ(ACT_OPEN_SETTINGS, config.button_actions[2]);
    ASSERT_EQ(UM_FAST_MONO_BAYER, config.update_mode);
    ASSERT_EQ(0, config.lightness);
    ASSERT_EQ(0, config.contrast);
    ASSERT_EQ(0, config.reserved_tone);
    ASSERT_EQ(AC_OFF, config.autoclear_mode);
    ASSERT_EQ(AC_5MIN, config.autoclear_interval);
    ASSERT_EQ(AC_THRES_MED, config.autoclear_threshold);
    ASSERT_EQ(0, config.osd_scale_2x);

    return 0;
}

static int test_legacy_config_validation_defaults_osd_scale_from_ppi(void) {
    config_init();

    config.hact = 1600;
    config.vact = 1200;
    config.size_x_mm = 135;
    config.size_y_mm = 101;
    config.osd_scale_2x = 0;
    config_validate_loaded(offsetof(config_t, osd_scale_2x));
    ASSERT_EQ(1, config.osd_scale_2x);

    config_init();
    config.hact = 1600;
    config.vact = 1200;
    config.size_x_mm = 270;
    config.size_y_mm = 203;
    config.osd_scale_2x = 1;
    config_validate_loaded(offsetof(config_t, osd_scale_2x));
    ASSERT_EQ(0, config.osd_scale_2x);

    return 0;
}

static int test_config_validation_repairs_invalid_values(void) {
    config_init();

    config.button_actions[0] = -1;
    config.button_actions[1] = ACT_COUNT;
    config.update_mode = 999;
    config.input_sel = 99;
    config.lightness = 4;
    config.contrast = -4;
    config.reserved_tone = 4;
    config.autoclear_mode = 99;
    config.autoclear_interval = -1;
    config.autoclear_threshold = 99;
    config.osd_scale_2x = 99;

    config_validate_loaded(sizeof(config));

    ASSERT_EQ(ACT_NEXT_MODE, config.button_actions[0]);
    ASSERT_EQ(ACT_PREV_MODE, config.button_actions[1]);
    ASSERT_EQ(UM_FAST_MONO_BAYER, config.update_mode);
    ASSERT_EQ(INPUT_SEL_AUTO, config.input_sel);
    ASSERT_EQ(3, config.lightness);
    ASSERT_EQ(-1, config.contrast);
    ASSERT_EQ(0, config.reserved_tone);
    ASSERT_EQ(AC_OFF, config.autoclear_mode);
    ASSERT_EQ(AC_5MIN, config.autoclear_interval);
    ASSERT_EQ(AC_THRES_MED, config.autoclear_threshold);
    ASSERT_EQ(1, config.osd_scale_2x);

    config.osd_scale_2x = -1;
    config_validate_loaded(sizeof(config));
    ASSERT_EQ(0, config.osd_scale_2x);

    config.contrast = 99;
    config_validate_loaded(sizeof(config));
    ASSERT_EQ(6, config.contrast);

    return 0;
}

static int test_menu_navigation_commits_and_cancels_modal_values(void) {
    ui_menu_t menu;

    config_init();
    ui_menu_init(&menu, &config);

    ASSERT_EQ(UI_MENU_DEPTH_CATEGORIES, ui_menu_depth(&menu));
    ASSERT_TRUE(ui_menu_selected_label(&menu) != NULL);
    ASSERT_EQ('D', ui_menu_selected_label(&menu)[0]);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(UI_MENU_DEPTH_ITEMS, ui_menu_depth(&menu));
    ASSERT_EQ('U', ui_menu_selected_label(&menu)[0]);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(UI_MENU_DEPTH_MODAL, ui_menu_depth(&menu));
    ASSERT_EQ(UM_FAST_MONO_BAYER, config.update_mode);

    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ASSERT_TRUE(ui_menu_modal_value_label(&menu) != NULL);
    ASSERT_EQ('W', ui_menu_modal_value_label(&menu)[0]);
    ui_menu_handle(&menu, UI_MENU_EVENT_BACK);
    ASSERT_EQ(UI_MENU_DEPTH_ITEMS, ui_menu_depth(&menu));
    ASSERT_EQ(UM_FAST_MONO_BAYER, config.update_mode);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(UI_MENU_DEPTH_ITEMS, ui_menu_depth(&menu));
    ASSERT_EQ(UM_FAST_MONO_BLUE_NOISE, config.update_mode);

    return 0;
}

static int test_menu_snapshot_exposes_visible_rows_and_values(void) {
    ui_menu_t menu;

    config_init();
    ui_menu_init(&menu, &config);

    ASSERT_EQ(3, ui_menu_row_count(&menu));
    ASSERT_EQ('D', ui_menu_row_label(&menu, 0)[0]);
    ASSERT_EQ('B', ui_menu_row_label(&menu, 1)[0]);
    ASSERT_EQ(1, ui_menu_row_selected(&menu, 0));

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(6, ui_menu_row_count(&menu));
    ASSERT_EQ('U', ui_menu_row_label(&menu, 0)[0]);
    ASSERT_EQ('B', ui_menu_row_value(&menu, 0)[0]);
    ASSERT_EQ(1, ui_menu_row_selected(&menu, 0));
    ASSERT_EQ('L', ui_menu_row_label(&menu, 1)[0]);
    ASSERT_EQ('C', ui_menu_row_label(&menu, 2)[0]);
    ASSERT_EQ('A', ui_menu_row_label(&menu, 3)[0]);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(4, ui_menu_row_count(&menu));
    ASSERT_EQ('B', ui_menu_row_label(&menu, 0)[0]);
    ASSERT_EQ('W', ui_menu_row_label(&menu, 1)[0]);
    ASSERT_EQ(1, ui_menu_row_selected(&menu, 0));

    return 0;
}

static int test_menu_scalar_modal_commits_lightness(void) {
    ui_menu_t menu;

    config_init();
    ui_menu_init(&menu, &config);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ASSERT_EQ('L', ui_menu_selected_label(&menu)[0]);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(UI_MENU_DEPTH_MODAL, ui_menu_depth(&menu));
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ASSERT_TRUE(ui_menu_modal_value_label(&menu) != NULL);
    ASSERT_EQ('1', ui_menu_modal_value_label(&menu)[0]);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);

    ASSERT_EQ(UI_MENU_DEPTH_ITEMS, ui_menu_depth(&menu));
    ASSERT_EQ(1, config.lightness);

    return 0;
}

static int test_menu_scalar_modal_exposes_preview_value_without_commit(void) {
    ui_menu_t menu;

    config_init();
    ui_menu_init(&menu, &config);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);

    ASSERT_EQ(1, ui_menu_modal_is_tone(&menu));
    ASSERT_EQ(1, ui_menu_modal_tone_target(&menu));
    ASSERT_EQ(0, ui_menu_modal_preview_value(&menu));

    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ASSERT_EQ(1, ui_menu_modal_preview_value(&menu));
    ASSERT_EQ(0, config.lightness);

    ui_menu_handle(&menu, UI_MENU_EVENT_BACK);
    ASSERT_EQ(UI_MENU_DEPTH_ITEMS, ui_menu_depth(&menu));
    ASSERT_EQ(0, config.lightness);

    return 0;
}

static int test_menu_contrast_uses_lcd_monitor_range(void) {
    ui_menu_t menu;

    config_init();
    ui_menu_init(&menu, &config);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ASSERT_EQ('C', ui_menu_selected_label(&menu)[0]);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(UI_MENU_DEPTH_MODAL, ui_menu_depth(&menu));
    ASSERT_EQ(1, ui_menu_modal_is_tone(&menu));
    ASSERT_EQ(2, ui_menu_modal_tone_target(&menu));
    ASSERT_EQ(8, ui_menu_modal_count(&menu));
    ASSERT_EQ(1, ui_menu_modal_index(&menu));
    ASSERT_EQ(0, ui_menu_modal_preview_value(&menu));

    ui_menu_handle(&menu, UI_MENU_EVENT_PREV);
    ASSERT_EQ(-1, ui_menu_modal_preview_value(&menu));
    ui_menu_handle(&menu, UI_MENU_EVENT_PREV);
    ASSERT_EQ(6, ui_menu_modal_preview_value(&menu));

    return 0;
}

static int test_menu_exposes_category_and_scalar_modal_metadata(void) {
    ui_menu_t menu;

    config_init();
    ui_menu_init(&menu, &config);

    ASSERT_EQ('D', ui_menu_category_label(&menu)[0]);
    ASSERT_EQ(0, ui_menu_modal_is_scalar(&menu));

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ('D', ui_menu_category_label(&menu)[0]);
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);

    ASSERT_EQ(UI_MENU_DEPTH_MODAL, ui_menu_depth(&menu));
    ASSERT_EQ(1, ui_menu_modal_is_scalar(&menu));
    ASSERT_EQ(7, ui_menu_modal_count(&menu));
    ASSERT_EQ(3, ui_menu_modal_index(&menu));
    ASSERT_EQ('0', ui_menu_modal_value_label(&menu)[0]);

    return 0;
}

static int test_menu_viewport_moves_lazily(void) {
    ASSERT_EQ(0, ui_menu_viewport_first(0, 0, 7, 4));
    ASSERT_EQ(0, ui_menu_viewport_first(0, 3, 7, 4));
    ASSERT_EQ(1, ui_menu_viewport_first(0, 4, 7, 4));
    ASSERT_EQ(4, ui_menu_viewport_first(4, 7, 8, 4));
    ASSERT_EQ(4, ui_menu_viewport_first(4, 6, 8, 4));
    ASSERT_EQ(3, ui_menu_viewport_first(4, 3, 8, 4));
    ASSERT_EQ(0, ui_menu_viewport_first(4, 0, 3, 4));

    return 0;
}

static int test_menu_autoclear_threshold_uses_frequency_labels(void) {
    ui_menu_t menu;

    config_init();
    ui_menu_init(&menu, &config);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    for (int i = 0; i < 5; i++)
        ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ASSERT_EQ('C', ui_menu_selected_label(&menu)[0]);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(UI_MENU_DEPTH_MODAL, ui_menu_depth(&menu));
    ASSERT_EQ('S', ui_menu_row_label(&menu, 0)[0]);
    ASSERT_EQ('O', ui_menu_row_label(&menu, 1)[0]);
    ASSERT_EQ('O', ui_menu_row_label(&menu, 2)[0]);
    ASSERT_EQ('f', ui_menu_row_label(&menu, 2)[1]);

    return 0;
}

static int test_system_menu_exposes_osd_scale_setting(void) {
    ui_menu_t menu;

    config_init();
    ui_menu_init(&menu, &config);

    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ASSERT_EQ('S', ui_menu_selected_label(&menu)[0]);

    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(UI_MENU_DEPTH_ITEMS, ui_menu_depth(&menu));
    ASSERT_EQ('I', ui_menu_row_label(&menu, 0)[0]);
    ASSERT_EQ('O', ui_menu_row_label(&menu, 1)[0]);
    ASSERT_EQ('1', ui_menu_row_value(&menu, 1)[0]);

    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(UI_MENU_DEPTH_MODAL, ui_menu_depth(&menu));
    ASSERT_EQ(2, ui_menu_row_count(&menu));
    ASSERT_EQ('1', ui_menu_row_label(&menu, 0)[0]);
    ASSERT_EQ('2', ui_menu_row_label(&menu, 1)[0]);
    ui_menu_handle(&menu, UI_MENU_EVENT_NEXT);
    ui_menu_handle(&menu, UI_MENU_EVENT_ENTER);
    ASSERT_EQ(1, config.osd_scale_2x);

    return 0;
}

static int test_per_mode_tone_note_and_recall(void) {
    config_init();

    // No tone stored anywhere: recall is a no-op for every mode.
    config.lightness = 1;
    config.contrast = 2;
    ASSERT_TRUE(!config_recall_tone_for_mode(UM_FAST_GREY));
    ASSERT_EQ(1, config.lightness);
    ASSERT_EQ(2, config.contrast);

    // Store a tone for FastGrey; other modes stay unset.
    config.lightness = -2;
    config.contrast = 5;
    config_note_tone_for_mode(UM_FAST_GREY);
    ASSERT_EQ(-2, config.tone_lightness[UM_FAST_GREY]);
    ASSERT_EQ(5, config.tone_contrast[UM_FAST_GREY]);
    ASSERT_EQ(TONE_UNSET, config.tone_lightness[UM_FAST_MONO_NO_DITHER]);

    // Switching to a mode with no stored tone leaves the current tone.
    ASSERT_TRUE(!config_recall_tone_for_mode(UM_FAST_MONO_NO_DITHER));
    ASSERT_EQ(-2, config.lightness);

    // Switching back to FastGrey recalls its tone.
    config.lightness = 0;
    config.contrast = 0;
    ASSERT_TRUE(config_recall_tone_for_mode(UM_FAST_GREY));
    ASSERT_EQ(-2, config.lightness);
    ASSERT_EQ(5, config.contrast);

    // Recalling an already-active tone is a no-op.
    ASSERT_TRUE(!config_recall_tone_for_mode(UM_FAST_GREY));

    // Validation keeps out-of-range stored tones from breaking bounds and
    // preserves TONE_UNSET sentinels.
    config.tone_lightness[UM_FAST_MONO_BAYER] = 99;
    config.tone_contrast[UM_FAST_MONO_BAYER] = 99;
    config.tone_lightness[UM_AUTO_LUT_ERROR_DIFFUSION] = TONE_UNSET;
    config.tone_contrast[UM_AUTO_LUT_ERROR_DIFFUSION] = TONE_UNSET;
    config_validate_loaded(sizeof(config_t));
    ASSERT_EQ(3, config.tone_lightness[UM_FAST_MONO_BAYER]);
    ASSERT_EQ(6, config.tone_contrast[UM_FAST_MONO_BAYER]);
    ASSERT_EQ(TONE_UNSET, config.tone_lightness[UM_AUTO_LUT_ERROR_DIFFUSION]);

    // Out-of-range mode indexes are rejected safely.
    config_note_tone_for_mode((update_mode_t)99);
    ASSERT_TRUE(!config_recall_tone_for_mode((update_mode_t)99));

    return 0;
}

int main(void) {
    int rc = 0;

    rc |= test_config_defaults_match_button_design();
    rc |= test_per_mode_tone_note_and_recall();
    rc |= test_legacy_config_validation_repairs_appended_fields();
    rc |= test_legacy_config_validation_defaults_osd_scale_from_ppi();
    rc |= test_config_validation_repairs_invalid_values();
    rc |= test_menu_navigation_commits_and_cancels_modal_values();
    rc |= test_menu_snapshot_exposes_visible_rows_and_values();
    rc |= test_menu_scalar_modal_commits_lightness();
    rc |= test_menu_scalar_modal_exposes_preview_value_without_commit();
    rc |= test_menu_contrast_uses_lcd_monitor_range();
    rc |= test_menu_exposes_category_and_scalar_modal_metadata();
    rc |= test_menu_viewport_moves_lazily();
    rc |= test_menu_autoclear_threshold_uses_frequency_labels();
    rc |= test_system_menu_exposes_osd_scale_setting();

    return rc;
}
