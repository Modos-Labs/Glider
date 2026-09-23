#include <stdint.h>
#include <stdio.h>

#include "config.h"
#include "edid.h"

#define EDID_DTD_PREFERRED_OFFSET 54
#define EDID_DESC_NAME_OFFSET     72
#define EDID_DESC_RANGE_OFFSET    90
#define EDID_DESC_DUMMY_OFFSET    108
#define EDID_BLOCK_SIZE           128
#define EDID_HDMI_SIZE            128

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("ASSERT_EQ failed at %s:%d: expected %lu got %lu\n", \
                __FILE__, __LINE__, (unsigned long)(expected), \
                (unsigned long)(actual)); \
        return 1; \
    } \
} while (0)

config_t config;

uint32_t board_get_uid(void) {
    return 0x00360049u;
}

static uint8_t checksum(const uint8_t *raw) {
    uint8_t sum = 0;

    for (size_t i = 0; i < EDID_BLOCK_SIZE; i++) {
        sum += raw[i];
    }
    return sum;
}

static uint32_t dtd_pclk_hz(const uint8_t *raw, size_t offset) {
    return ((uint32_t)raw[offset] | ((uint32_t)raw[offset + 1] << 8)) *
            10000u;
}

static uint32_t dtd_hact(const uint8_t *raw, size_t offset) {
    return (uint32_t)raw[offset + 2] |
            ((uint32_t)(raw[offset + 4] & 0xf0) << 4);
}

static uint32_t dtd_hblk(const uint8_t *raw, size_t offset) {
    return (uint32_t)raw[offset + 3] |
            ((uint32_t)(raw[offset + 4] & 0x0f) << 8);
}

static uint32_t dtd_vact(const uint8_t *raw, size_t offset) {
    return (uint32_t)raw[offset + 5] |
            ((uint32_t)(raw[offset + 7] & 0xf0) << 4);
}

static uint32_t dtd_vblk(const uint8_t *raw, size_t offset) {
    return (uint32_t)raw[offset + 6] |
            ((uint32_t)(raw[offset + 7] & 0x0f) << 8);
}

static uint32_t dtd_hfp(const uint8_t *raw, size_t offset) {
    return (uint32_t)raw[offset + 8] |
            ((uint32_t)(raw[offset + 11] & 0xc0) << 2);
}

static uint32_t dtd_hsync(const uint8_t *raw, size_t offset) {
    return (uint32_t)raw[offset + 9] |
            ((uint32_t)(raw[offset + 11] & 0x30) << 4);
}

static uint32_t dtd_vfp(const uint8_t *raw, size_t offset) {
    return (uint32_t)(raw[offset + 10] >> 4) |
            ((uint32_t)(raw[offset + 11] & 0x0c) << 2);
}

static uint32_t dtd_vsync(const uint8_t *raw, size_t offset) {
    return (uint32_t)(raw[offset + 10] & 0x0f) |
            ((uint32_t)(raw[offset + 11] & 0x03) << 4);
}

static int expect_range_descriptor(const uint8_t *raw, size_t offset) {
    ASSERT_EQ(0x00u, raw[offset + 0]);
    ASSERT_EQ(0x00u, raw[offset + 1]);
    ASSERT_EQ(0x00u, raw[offset + 2]);
    ASSERT_EQ(0xfdu, raw[offset + 3]);
    ASSERT_EQ(0x00u, raw[offset + 4]);
    ASSERT_EQ(48u, raw[offset + 5]);
    ASSERT_EQ(85u, raw[offset + 6]);
    ASSERT_EQ(30u, raw[offset + 7]);
    ASSERT_EQ(90u, raw[offset + 8]);
    ASSERT_EQ(170u / 10u, raw[offset + 9]);

    return 0;
}

static int expect_configured_timing(const uint8_t *raw, size_t offset) {
    ASSERT_EQ(134000000u, dtd_pclk_hz(raw, offset));
    ASSERT_EQ(1448u, dtd_hact(raw, offset));
    ASSERT_EQ(160u, dtd_hblk(raw, offset));
    ASSERT_EQ(1072u, dtd_vact(raw, offset));
    ASSERT_EQ(39u, dtd_vblk(raw, offset));
    ASSERT_EQ(48u, dtd_hfp(raw, offset));
    ASSERT_EQ(32u, dtd_hsync(raw, offset));
    ASSERT_EQ(3u, dtd_vfp(raw, offset));
    ASSERT_EQ(10u, dtd_vsync(raw, offset));

    return 0;
}

static void init_test_config(void) {
    config.pclk_hz = 134000000u;
    config.hfp = 48;
    config.hsync = 32;
    config.hact = 1448;
    config.hblk = 160;
    config.vfp = 3;
    config.vsync = 10;
    config.vact = 1072;
    config.vblk = 39;
    config.size_x_mm = 122;
    config.size_y_mm = 91;
    config.mfg_week = 27;
    config.mfg_year = 0x24;
}

static int test_edid_conformity_fields(void) {
    const uint8_t *raw = edid_get_raw();
    const uint8_t *hdmi_raw = edid_get_raw_hdmi();

    ASSERT_EQ(EDID_HDMI_SIZE, edid_get_raw_hdmi_size());
    ASSERT_EQ(0u, checksum(raw));
    ASSERT_EQ(0u, checksum(hdmi_raw));
    ASSERT_EQ(0x81u, raw[20]);
    ASSERT_EQ(0x0au, raw[24]);
    ASSERT_EQ(12u, raw[21]);
    ASSERT_EQ(10u, raw[22]);
    ASSERT_EQ(0u, raw[126]);
    ASSERT_EQ(0u, hdmi_raw[126]);

    for (size_t i = 38; i < 54; i += 2) {
        ASSERT_EQ(0x01u, raw[i]);
        ASSERT_EQ(0x01u, raw[i + 1]);
    }

    ASSERT_EQ(0xfcu, raw[EDID_DESC_NAME_OFFSET + 3]);
    ASSERT_EQ('P', raw[EDID_DESC_NAME_OFFSET + 5]);
    ASSERT_EQ('M', raw[EDID_DESC_NAME_OFFSET + 11]);

    if (expect_range_descriptor(raw, EDID_DESC_RANGE_OFFSET) != 0)
        return 1;

    ASSERT_EQ(0x00u, raw[EDID_DESC_DUMMY_OFFSET + 0]);
    ASSERT_EQ(0x00u, raw[EDID_DESC_DUMMY_OFFSET + 1]);
    ASSERT_EQ(0x00u, raw[EDID_DESC_DUMMY_OFFSET + 2]);
    ASSERT_EQ(0x10u, raw[EDID_DESC_DUMMY_OFFSET + 3]);

    return 0;
}

static int test_configured_timing(void) {
    const uint8_t *raw = edid_get_raw();
    const uint8_t *hdmi_raw = edid_get_raw_hdmi();

    if (expect_configured_timing(raw, EDID_DTD_PREFERRED_OFFSET) != 0)
        return 1;
    if (expect_configured_timing(hdmi_raw, EDID_DTD_PREFERRED_OFFSET) != 0)
        return 1;

    return 0;
}

int main(int argc, char **argv) {
    int rc = 0;

    init_test_config();
    edid_init();

    rc |= test_edid_conformity_fields();
    rc |= test_configured_timing();

    if ((rc == 0) && (argc == 2)) {
        FILE *f = fopen(argv[1], "wb");
        if (f == NULL) {
            perror("fopen");
            return 1;
        }
        if (fwrite(edid_get_raw_hdmi(), 1, edid_get_raw_hdmi_size(), f) !=
                edid_get_raw_hdmi_size()) {
            perror("fwrite");
            fclose(f);
            return 1;
        }
        fclose(f);
    }

    return rc;
}
