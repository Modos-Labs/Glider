//
// Grimoire
// Copyright 2025 Wenting Zhang
//
// Original copyright information:
// Copyright (c) 2022 - Analog Devices Inc. All Rights Reserved.
//
// This file is licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.  You may
// obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "platform.h"
#include "board.h"
#include "shell.h"
#include "shell_printf.h"
#include "term.h"
#include "app_main.h"
#include "app.h"
#include "xmodem.h"

#ifdef printf
#undef printf
#endif

#ifdef vprintf
#undef vprintf
#endif

#define printf(...) shell_printf(ctx, __VA_ARGS__)
#define vprintf(x,y) shell_vprintf(ctx, x, y)

/***********************************************************************
 * Misc helper functions
 **********************************************************************/
static void dump_bytes(shell_context_t *ctx, unsigned char *rdata, unsigned addr, unsigned rlen) {
    unsigned i;
    for (i = 0; i < rlen; i++) {
        if ((i % 16) == 0) {
            if (i) {
                printf("\n");
            }
            printf("%08x: ", addr + i);
        }
        printf("%02x ", rdata[i]);
    }
    printf("\n");
}

int confirm_danger(shell_context_t *ctx, char *warnStr) {
    char c;

    printf( "%s\n", warnStr );
    printf( "Are you sure you want to continue? [y/n]" );

    c = term_getch( &ctx->t, TERM_INPUT_WAIT );
    printf( "%c\n", isprint( c ) ? c : ' ' );

    if( tolower(c) == 'y' ) {
        return(1);
    }

    return(0);
}

/***********************************************************************
 * CMD: syslog
 **********************************************************************/
const char shell_help_syslog[] = "\n";
const char shell_help_summary_syslog[] = "Show the live system log";

#define MAX_TS_LINE  32
#define MAX_LOG_LINE 256

void shell_syslog(shell_context_t *ctx, int argc, char **argv) {
    char *line;
    char *lbuf;
    char *ts;
    int c;

    ts = SHELL_MALLOC(MAX_TS_LINE);
    lbuf = SHELL_MALLOC(MAX_LOG_LINE);

    c = 0;
    do {
        line = syslog_next(ts, MAX_TS_LINE, lbuf, MAX_LOG_LINE);
        if (line) {
            printf("%s %s\n", ts, line);
        }
        c = term_getch(&ctx->t, TERM_INPUT_DONT_WAIT);
#ifdef FREE_RTOS
        if ((line == NULL) && (c < 0)) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
#endif
    } while (c < 0);

    if (ts) {
        SHELL_FREE(ts);
    }
    if (lbuf) {
        SHELL_FREE(lbuf);
    }
}

/***********************************************************************
 * CMD: stacks
 **********************************************************************/
const char shell_help_stacks[] = "\n";
const char shell_help_summary_stacks[] = "Report task stack usage";

static void shell_print_task_stack(shell_context_t *ctx, TaskHandle_t task) {
    if (task) {
        printf(" %s: %u\n",
            pcTaskGetName(task), (unsigned)uxTaskGetStackHighWaterMark(task)
        );
    }
}

void shell_stacks(shell_context_t *ctx, int argc, char **argv ) {
    printf("High Water Marks are in 32-bit words (zero is bad).\n");
    printf("Task Stask High Water Marks:\n");

    shell_print_task_stack(ctx, startup_task_handle);
    shell_print_task_stack(ctx, housekeeping_task_handle);
    shell_print_task_stack(ctx, usb_device_task_handle);
    shell_print_task_stack(ctx, usb_pd_task_handle);
    shell_print_task_stack(ctx, ui_task_handle);
    shell_print_task_stack(ctx, key_scan_task_handle);
    shell_print_task_stack(ctx, power_mon_task_handle);
}

/***********************************************************************
 * CMD: test
 **********************************************************************/
const char shell_help_test[] = "\n";
const char shell_help_summary_test[] = "Test command";

void shell_test(shell_context_t *ctx, int argc, char **argv ) {

//    uint8_t val = atoi(argv[1]);
//    uint8_t buf[2];
//    buf[0] = 0x19;
//    buf[1] = val | 0x80;
//     pal_i2c_write_payload(ADV7611_I2C, ADV7611_I2C_ADDR, buf, 2);

//    uint8_t val;
//
//    val = adv7611_read_reg(ADV7611_I2C_ADDR, 0x6a);
//    if (val & 0x10)
//        printf("TMDS clock detected\n");
//    else
//        printf("No TMDS clock detected\n");
//
//    uint16_t val16;
//    val = adv7611_read_reg(HDMI_I2C_ADDR, 0x51); // D8-D1
//    val16 = (uint16_t)val << 1;
//    val = adv7611_read_reg(HDMI_I2C_ADDR, 0x52);
//    val16 |= val >> 7;
//
//    printf("TMDS frequency %d MHz\n", val16);
//
//    val = adv7611_read_reg(HDMI_I2C_ADDR, 0x04);
//    if (val & 0x2)
//        printf("TMDS PLL locked\n");
//    else
//        printf("TMDS PLL not locked\n");
//
//    val = adv7611_read_reg(HDMI_I2C_ADDR, 0x05);
//    if (val & 0x80)
//        printf("HDMI mode detected\n");
//    else
//        printf("DVI mode detected\n");
//
//    val = adv7611_read_reg(HDMI_I2C_ADDR, 0x07);
//    if (val & 0x20)
//        printf("DE regeneration locked\n");
//    else
//        printf("DE regeneration not locked\n");
//
//    if (val & 0x80)
//        printf("Vertical filter locked\n");
//    else
//        printf("Vertical filter not locked\n");
//
//    val16 = (uint16_t)adv7611_read_reg(HDMI_I2C_ADDR, 0x1e) << 8;
//    val16 |= adv7611_read_reg(HDMI_I2C_ADDR, 0x1f);
//    printf("Total line width: %d\n", val16);
//
//    val16 = (uint16_t)(adv7611_read_reg(HDMI_I2C_ADDR, 0x07) & 0x1f) << 8;
//    val16 |= adv7611_read_reg(HDMI_I2C_ADDR, 0x08);
//    printf("Active line width: %d\n", val16);
//
//    val16 = (uint16_t)(adv7611_read_reg(HDMI_I2C_ADDR, 0x09) & 0x1f) << 8;
//    val16 |= adv7611_read_reg(HDMI_I2C_ADDR, 0x0a);
//    printf("Active field height: %d\n", val16);
}

/***********************************************************************
 * CMD: i2c_probe
 **********************************************************************/
const char shell_help_i2c_probe[] = "<i2c_port>\n"
  "  i2c_port - I2C port to probe\n";
const char shell_help_summary_i2c_probe[] = "Probe an I2C port for active devices";

void shell_i2c_probe(shell_context_t *ctx, int argc, char **argv)
{
    pal_i2c_t *i2c;
    int i;

    if (argc != 2) {
        printf("Invalid arguments. Type help [<command>] for usage.\n");
        return;
    }

    int i2c_port = strtol(argv[1], NULL, 0);
    i2c = NULL;

    if (i2c_port == 1) {
        i2c = &pi2c1;
    }
    else {
        printf("Invalid I2C port!\n");
        return;
    }

    printf("Probing I2C port %d:\n", i2c_port);
    for (i = 0x08; i < 0x78; i++) {
        bool result = pal_i2c_ping(i2c, i);
        if (result == true) {
            printf(" Found device 0x%02x\n", i);
        }
    }
}


//const char shell_help_fl[] = "<operation> [brightness]\n"
//  "  operation - on | off | set\n"
//  "  brightness - 0-255\n";
//const char shell_help_summary_fl[] = "Control front light";
//
//void shell_fl(shell_context_t *ctx, int argc, char **argv)
//{
//    if ((argc < 2) || (argc > 3)) {
//        printf("Invalid arguments. Type help [<command>] for usage.\n");
//        return;
//    }
//
//    if (strcmp(argv[1], "on") == 0) {
//    	power_on_fl();
//    }
//    else if (strcmp(argv[1], "off") == 0) {
//    	power_off_fl();
//    }
//
//    if (argc == 3) {
//    	int brightness = strtol(argv[2], NULL, 0);
//    	power_set_fl_brightness(brightness);
//    }
//}


/***********************************************************************
 * XMODEM helper functions
 **********************************************************************/
typedef struct {
    shell_context_t *ctx;
} xmodem_state_t;

static void shell_xmodem_putchar(int c, void *usr) {
    xmodem_state_t *x = (xmodem_state_t *)usr;
    shell_context_t *ctx = x->ctx;
    term_state_t *t = &ctx->t;

    term_putch(t, c);
}

static int shell_xmodem_getchar(int timeout, void *usr) {
    xmodem_state_t *x = (xmodem_state_t *)usr;
    shell_context_t *ctx = x->ctx;
    term_state_t *t = &ctx->t;
    int c;

    c = term_getch(t, timeout);

    return(c);
}

typedef struct {
    xmodem_state_t xmodem;
    spiffs_file f;
    void *data;
    int size;
} file_xfer_state_t;

static void file_data_write(void *usr, void *data, int size) {
    file_xfer_state_t *state = (file_xfer_state_t *)usr;

    /*
     * Need to double-buffer the data in order to strip off the
     * trailing packet bytes at the end of an xmodem transfer.
     */
    if (state->data == NULL) {
        state->data = SHELL_MALLOC(1024);
        memcpy(state->data, data, size);
        state->size = size;
    } else {
        if (data) {
            SPIFFS_write(&spiffs_fs, state->f, state->data, state->size);
            memcpy(state->data, data, size);
            state->size = size;
        } else {
            uint8_t *buf = (uint8_t *)state->data;
            while (state->size && buf[state->size-1] == '\x1A') {
               state->size--;
            }
            SPIFFS_write(&spiffs_fs, state->f, state->data, state->size);
            if (state->data) {
                SHELL_FREE(state->data);
                state->data = NULL;
            }
        }
    }
}

static void file_data_read(void *usr, void *data, int size) {
    file_xfer_state_t *state = (file_xfer_state_t *)usr;

    if (size > 0) {
        SPIFFS_read(&spiffs_fs, state->f, data, size);
    }
}

/***********************************************************************
 * CMD: format
 **********************************************************************/
const char shell_help_format[] = "";
const char shell_help_summary_format[] = "Formats an internal flash filesystem";

void shell_format(shell_context_t *ctx, int argc, char **argv) {
    printf("Be patient, this may take a while.\n");
    printf("Formatting...\n");

    if (SPIFFS_format(&spiffs_fs) != SPIFFS_OK) {
        printf("SPIFFS format failed: %d\n", SPIFFS_errno(&spiffs_fs));
    }

    printf("Done.\n");
}

/***********************************************************************
 * CMD: rm/del
 **********************************************************************/
const char shell_help_rm[] = "<file1> [<file2> ...]\n";
const char shell_help_summary_rm[] = "Removes a file";

#include <stdio.h>

void shell_rm(shell_context_t *ctx, int argc, char **argv) {
    int i;

    if (argc < 2) {
        printf( "Usage: rm <file1> [<file2> ...]\n" );
        return;
    }

    for (i = 1; i < argc; i++) {
        if (SPIFFS_remove(&spiffs_fs, argv[i]) != 0) {
          printf("Unable to remove '%s'\n", argv[i]);
        }
    }
}

//const char shell_help_dump[] = "[addr] <len>\n"
//    "  addr - Starting address to dump\n"
//    "  len - Number of bytes to dump (default 1)\n";
//const char shell_help_summary_dump[] = "Hex dump of flash contents";
//
//void shell_dump(shell_context_t *ctx, int argc, char **argv)
//{
//    uintptr_t addr = 0;
//    unsigned len = 1;
//    uint8_t *buf;
//    int ok;
//
//    if (argc < 2) {
//        printf("Invalid arguments\n");
//        return;
//    }
//
//    addr = strtoul(argv[1], NULL, 0);
//    if (argc > 2) {
//        len = strtoul(argv[2], NULL, 0);
//    }
//
//    buf = SHELL_MALLOC(len);
//    if (buf) {
//        ok = spif_read(addr, len, buf);
//        if (ok == 0) {
//            dump_bytes(ctx, buf, addr, len);
//        }
//        SHELL_FREE(buf);
//    }
//}
//
const char shell_help_fdump[] =
 "[file] <start> <size>\n"
 "  file - File to dump\n"
 "  start - Start offset in bytes (default: 0)\n"
 "  size - Size in bytes (default: full file)\n";
const char shell_help_summary_fdump[] = "Dumps the contents of a file in hex";

#define DUMP_SIZE 512

void shell_fdump(shell_context_t *ctx, int argc, char **argv) {
    spiffs_file f;
    size_t start = 0;
    size_t size = SIZE_MAX;
    uint8_t *buf = NULL;
    size_t rlen;
    int c;

    if( argc < 2 ) {
        printf("No file given\n");
        return;
    }

    if (argc > 2) {
        start = (size_t)strtoul(argv[2], NULL, 0);
    }

    if (argc > 3) {
        size = (size_t)strtoul(argv[3], NULL, 0);
    }

    f = SPIFFS_open(&spiffs_fs, argv[1], SPIFFS_O_RDONLY, 0);
    if (f) {
        buf = SHELL_MALLOC(DUMP_SIZE);
        SPIFFS_lseek(&spiffs_fs, f, start, SPIFFS_SEEK_SET);
        do {
            rlen = (size < DUMP_SIZE) ? size : DUMP_SIZE;
            rlen = SPIFFS_read(&spiffs_fs, f, buf, rlen);
            if (rlen) {
                dump_bytes(ctx, buf, start, rlen);
                size -= rlen;
                start += rlen;
                c = term_getch(&ctx->t, TERM_INPUT_DONT_WAIT);
            }
        } while (size && rlen && (c < 0));
        if (buf) {
            SHELL_FREE(buf);
        }
        SPIFFS_close(&spiffs_fs, f);
    }
    else {
        printf("Unable to open '%s'\n", argv[1]);
    }
}

/***********************************************************************
 * CMD: recv
 **********************************************************************/
const char shell_help_recv[] = "<file>\n"
    "  Transfer and save to file\n";
const char shell_help_summary_recv[] = "Receive a file via XMODEM";

void shell_recv( shell_context_t *ctx, int argc, char **argv )
{
    file_xfer_state_t file_state = { 0 };
    long size;

    if( argc != 2 ) {
        printf( "Usage: recv <file>\n" );
        return;
    }

    file_state.xmodem.ctx = ctx;

    file_state.f = SPIFFS_open(&spiffs_fs, argv[1], SPIFFS_O_CREAT | SPIFFS_O_TRUNC | SPIFFS_O_WRONLY, 0);
    if( file_state.f == 0) {
        printf( "unable to open file %s\n", argv[ 1 ] );
        return;
    }
    printf( "Prepare your terminal for XMODEM send ... " );
    term_set_mode(&ctx->t, TERM_MODE_COOKED, 0);
    size = XmodemReceiveCrc(file_data_write, &file_state, 4*1024*1024,
        shell_xmodem_getchar, shell_xmodem_putchar);
    term_set_mode(&ctx->t, TERM_MODE_COOKED, 1);
    if (size < 0) {
        printf( "XMODEM Error: %ld\n", size);
    } else {
        printf( "received and saved as %s\n", argv[ 1 ] );
    }
    SPIFFS_close(&spiffs_fs, file_state.f);
}

/***********************************************************************
 * CMD: send
 **********************************************************************/
const char shell_help_send[] = "<file1> [<file2> ...]\n";
const char shell_help_summary_send[] = "Send files via YMODEM.";

typedef struct {
    file_xfer_state_t state;
    const char *fname;
    size_t fsize;
} ymodem_state_t;

static void ymodem_hdr(void *usr, void *xmodemBuffer, int xmodemSize) {
    ymodem_state_t *y = (ymodem_state_t *)usr;
    snprintf(xmodemBuffer, xmodemSize, "%s%c%u", y->fname, 0, (unsigned)y->fsize);
}

static void ymodem_end(void *xs, void *xmodemBuffer, int xmodemSize) {
}

void shell_send(shell_context_t *ctx, int argc, char **argv)
{
    size_t size;
    spiffs_file fp = 0;
    int ret = -1;
    int i;

    ymodem_state_t y = {
        .state.xmodem.ctx = ctx,
    };

    if (argc < 2) {
        printf("Usage: %s <file1> [<file2> ...]\n", argv[0]);
        return;
    }

    printf ("Prepare your terminal for YMODEM receive...\n");
    term_set_mode(&ctx->t, TERM_MODE_COOKED, 0);
    for (i = 1; i < argc; i++) {
        fp = SPIFFS_open(&spiffs_fs, argv[i], SPIFFS_O_RDONLY, 0);
        if (fp) {
            spiffs_stat s;
            SPIFFS_fstat(&spiffs_fs, fp, &s);
            size = s.size;
            y.fname = argv[i]; y.fsize = size; y.state.f = fp;
            ret = XmodemTransmit(ymodem_hdr, &y, 128, 0, 1,
                shell_xmodem_getchar, shell_xmodem_putchar);
            if (ret >= 0) {
                ret = XmodemTransmit(file_data_read, &y, y.fsize, 1, 0,
                    shell_xmodem_getchar, shell_xmodem_putchar);
            }
            SPIFFS_close(&spiffs_fs, fp);
            if (ret < 0) {
                break;
            }
        }
    }
    if (ret >= 0) {
        ret = XmodemTransmit(ymodem_end, &y, 128, 0, 1,
            shell_xmodem_getchar, shell_xmodem_putchar);
    }
    term_set_mode(&ctx->t, TERM_MODE_COOKED, 1);
    if (ret < 0) {
        printf( "YMODEM Error: %ld\n", ret);
    }
}


/***********************************************************************
 * CMD: df
 **********************************************************************/
const char shell_help_df[] = "\n";
const char shell_help_summary_df[] = "Shows internal filesystem disk full status";

void shell_df(shell_context_t *ctx, int argc, char **argv) {
    printf("%10s %10s %10s %5s\n", "Size", "Used", "Available", "Use %");

    int32_t serr; uint32_t ssize; uint32_t sused;
    serr = SPIFFS_info(&spiffs_fs, &ssize, &sused);
    if (serr == SPIFFS_OK) {
        printf("%10u %10u %10u %5u\n",
            (unsigned)ssize, (unsigned)sused, (unsigned)(ssize - sused),
            (unsigned)((100 * sused) / ssize));
    }
}


const char shell_help_setvolt[] = "<rail> <volt>\n";
const char shell_help_summary_setvolt[] = "Set voltage";

void shell_setvolt(shell_context_t *ctx, int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: <rail> <volt>\n", argv[0]);
        return;
    }

    float volt = strtof(argv[2], NULL);
    if (strcmp(argv[1], "vcom") == 0) {
        power_set_vcom(volt);
    }
    else if (strcmp(argv[1], "vgh") == 0) {
        power_set_vgh(volt);
    }
}


const char shell_help_setcfg[] = "<set|get|save> [key] [value]\n";
const char shell_help_summary_setcfg[] = "Sets configuration. Remember to use save to save it to the flash.";

typedef struct {
    const char *name;
    void *pointer;
    enum {UINT8, UINT16, UINT32, FLOAT32} type;
} cfg_var_t;

cfg_var_t vars[] = {
    {"pclk_hz", &(config.pclk_hz), UINT32},
    {"hfp", &(config.hfp), UINT8},
    {"vfp", &(config.vfp), UINT8},
    {"hsync", &(config.hsync), UINT8},
    {"vsync", &(config.vsync), UINT8},
    {"hact", &(config.hact), UINT16},
    {"hblk", &(config.hblk), UINT16},
    {"vact", &(config.vact), UINT16},
    {"vblk", &(config.vblk), UINT16},
    {"size_x_mm", &(config.size_x_mm), UINT16},
    {"size_y_mm", &(config.size_y_mm), UINT16},
    {"mfg_week", &(config.mfg_week), UINT8},
    {"mfg_year", &(config.mfg_year), UINT8},
    {"vcom", &(config.vcom), FLOAT32},
    {"vgh", &(config.vgh), FLOAT32},
    {"tcon_vfp", &(config.tcon_vfp), UINT8},
    {"tcon_vsync", &(config.tcon_vsync), UINT8},
    {"tcon_vbp", &(config.tcon_vbp), UINT8},
    {"tcon_vact", &(config.tcon_vact), UINT16},
    {"tcon_hfp", &(config.tcon_hfp), UINT8},
    {"tcon_hsync", &(config.tcon_hsync), UINT8},
    {"tcon_hbp", &(config.tcon_hbp), UINT8},
    {"tcon_hact", &(config.tcon_hact), UINT16},
    {"mirror", &(config.mirror), UINT8}
};
int num_vars = sizeof(vars) / sizeof(cfg_var_t);

static void setcfg_set_helper(cfg_var_t *var, char *val) {
    if (var->type == UINT8) {
        *(uint8_t *)(var->pointer) = strtol(val, NULL, 10);
    }
    else if (var->type == UINT16) {
        *(uint16_t *)(var->pointer) = strtol(val, NULL, 10);
    }
    else if (var->type == UINT32) {
        *(uint32_t *)(var->pointer) = strtol(val, NULL, 10);
    }
    else if (var->type == FLOAT32) {
        *(float *)(var->pointer) = strtof(val, NULL);
    }
}

static void setcfg_get_helper(shell_context_t *ctx, cfg_var_t *var) {
    if (var->type == UINT8) {
        printf("%d\n", *(uint8_t *)(var->pointer));
    }
    else if (var->type == UINT16) {
        printf("%d\n", *(uint16_t *)(var->pointer));
    }
    else if (var->type == UINT32) {
        printf("%d\n", *(uint32_t *)(var->pointer));
    }
    else if (var->type == FLOAT32) {
        printf("%f\n", *(float *)(var->pointer));
    }
}

void shell_setcfg(shell_context_t *ctx, int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s\n", shell_help_setcfg);
        return;
    }

    cfg_var_t *var = NULL;
    if (argc >= 3) {
        for (int i = 0; i < num_vars; i++) {
            if (strcmp(argv[2], vars[i].name) == 0) {
                var = &vars[i];
            }
        }
        if (var == NULL) {
            printf("Unknown key %s", argv[2]);
            return;
        }
    }

    if (strcmp(argv[1], "set") == 0) {
        if (argc < 4) {
            printf("Key and value required for set\n");
            return;
        }
        setcfg_set_helper(var, argv[3]);
    }
    else if (strcmp(argv[1], "get") == 0) {
        if (argc < 4) {
            // Get every var
            for (int i = 0; i < num_vars; i++) {
                printf("%s: ", vars[i].name);
                setcfg_get_helper(ctx, &vars[i]);
            }
        }
        else {
            setcfg_get_helper(ctx, var);
        }
    }
    else if (strcmp(argv[1], "save") == 0) {
        config_save();
    }
}

const char shell_help_sensor[] = "\n";
const char shell_help_summary_sensor[] = "Get sensor readings";

void shell_sensor(shell_context_t *ctx, int argc, char **argv) {
    printf("EPD Supplies:\n");
    printf("VP:     %5.2f V\n", power_get_rail_voltage(RAIL_VP));
    printf("VGH:    %5.2f V\n", power_get_rail_voltage(RAIL_VGH));
    printf("VN:     %5.2f V\n", power_get_rail_voltage(RAIL_VN));
    printf("VGL:    %5.2f V\n", power_get_rail_voltage(RAIL_VGL));
    printf("VCOM:   %5.2f V\n", power_get_rail_voltage(RAIL_VCOM));
    printf("5VES:   %5.2f V %3.0f mA\n", power_get_rail_voltage(RAIL_5VES), power_get_rail_current(RAIL_5VES));
    printf("5VEG:   %5.2f V %3.0f mA\n", power_get_rail_voltage(RAIL_5VEG), power_get_rail_current(RAIL_5VEG));

    printf("System Supplies:\n");
    printf("VBUS:   %5.2f V\n", power_get_rail_voltage(RAIL_VBUS));
    printf("3V3:    %5.2f V %3.0f mA\n", power_get_rail_voltage(RAIL_3V3), power_get_rail_current(RAIL_3V3));
    printf("1V8VID: %5.2f V %3.0f mA\n", power_get_rail_voltage(RAIL_1V8VID), power_get_rail_current(RAIL_1V8VID));
    printf("3V3VID: %5.2f V %3.0f mA\n", power_get_rail_voltage(RAIL_3V3VID), power_get_rail_current(RAIL_3V3VID));
    printf("5V2FL:  %5.2f V %3.0f mA\n", power_get_rail_voltage(RAIL_5V2FL), power_get_rail_current(RAIL_5V2FL));
    printf("1V35:   %5.2f V %3.0f mA\n", power_get_rail_voltage(RAIL_1V35), power_get_rail_current(RAIL_1V35));
    printf("1V2:    %5.2f V %3.0f mA\n", power_get_rail_voltage(RAIL_1V2), power_get_rail_current(RAIL_1V2));

    float p_cur, p_avg, p_max;
    float p_cur_sum, p_avg_sum, p_max_sum;

    printf("Power Consumption (CUR, AVG, MAX):\n");
    power_get_rail_power(RAIL_3V3, &p_cur, &p_avg, &p_max);
    printf("MCU + IO:  %5.1f mW  %5.1f mW  %5.1f mW\n", p_cur, p_avg, p_max);
    power_get_rail_power(RAIL_1V35, &p_cur, &p_avg, &p_max);
    printf("FPGA DDR:  %5.1f mW  %5.1f mW  %5.1f mW\n", p_cur, p_avg, p_max);
    power_get_rail_power(RAIL_1V2, &p_cur, &p_avg, &p_max);
    printf("FPGA CORE: %5.1f mW  %5.1f mW  %5.1f mW\n", p_cur, p_avg, p_max);
    power_get_rail_power(RAIL_1V8VID, &p_cur_sum, &p_avg_sum, &p_max_sum);
    power_get_rail_power(RAIL_3V3VID, &p_cur, &p_avg, &p_max);
    p_cur_sum += p_cur; p_avg_sum += p_avg; p_max_sum += p_max;
    printf("VIDEO IN:  %5.1f mW  %5.1f mW  %5.1f mW\n", p_cur_sum, p_avg_sum, p_max_sum);
    power_get_rail_power(RAIL_5VES, &p_cur_sum, &p_avg_sum, &p_max_sum);
    power_get_rail_power(RAIL_5VEG, &p_cur, &p_avg, &p_max);
    p_cur_sum += p_cur; p_avg_sum += p_avg; p_max_sum += p_max;
    printf("EPD HV:    %5.1f mW  %5.1f mW  %5.1f mW\n", p_cur_sum, p_avg_sum, p_max_sum);
}
