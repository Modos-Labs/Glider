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
#include "syslog.h"

// The maximum number of entries in the syslog FIFO
#define SYSLOG_MAX_LINES   (100)
// The maximum line length of a syslog entry
#define SYSLOG_LINE_MAX    (128)
// The function used to allocate memory for the syslog buffer.
#define SYSLOG_MALLOC(x)   pvPortMalloc(x)
// The function used to free memory.
#define SYSLOG_FREE(x)     vPortFree(x)
// The name of the syslog instance.
#define SYSLOG_NAME        "Console Log"

#define SYSLOG_CRITICAL_ENTRY(x)    xSemaphoreTake(x, portMAX_DELAY);
#define SYSLOG_CRITICAL_EXIT(x)     xSemaphoreGive(x)

/* TODO: abstract out mutex create/destroy/type */

typedef struct {
    SemaphoreHandle_t lock;
    char *name;
    uint32_t head_idx;
    uint32_t tail_idx;
    uint32_t seq;
    int32_t log_dropped;
    uint64_t *ts_data;
    char *log_data;
} syslog_context_t;

syslog_context_t consoleLog = {
    .name = SYSLOG_NAME
};

void syslog_init(void) {
    syslog_context_t *log = &consoleLog;

    log->lock = xSemaphoreCreateMutex();
    log->head_idx = 0;
    log->tail_idx = 0;
    log->log_dropped = 0;
    log->seq = 0;
    log->log_data = SYSLOG_MALLOC(SYSLOG_MAX_LINES * SYSLOG_LINE_MAX);
    log->ts_data = SYSLOG_MALLOC(SYSLOG_MAX_LINES * sizeof(*(log->ts_data)));
}

void syslog_print(char *msg)
{
    syslog_context_t *log = &consoleLog;
    char *start, *end;

    SYSLOG_CRITICAL_ENTRY(log->lock);

    log->head_idx++;
    if (log->head_idx == SYSLOG_MAX_LINES) {
        log->head_idx = 0;
    }

    if (log->head_idx == log->tail_idx) {
        log->tail_idx++;
        if (log->tail_idx == SYSLOG_MAX_LINES) {
            log->tail_idx = 0;
        }
        log->log_dropped++;
    }

    start = &log->log_data[log->head_idx * SYSLOG_LINE_MAX];
    end = start + SYSLOG_LINE_MAX - 1;
    strncpy(start, msg, SYSLOG_LINE_MAX); *end = 0;
    log->ts_data[log->head_idx] = xTaskGetTickCount() / portTICK_PERIOD_MS;

    log->seq++;

//     char *rdptr = msg;
//     while (*rdptr) {
//     	usbapp_term_out(*rdptr++);
//     }
//     usbapp_term_out('\n');
//     usbapp_term_out('\r');

    SYSLOG_CRITICAL_EXIT(log->lock);
}

void syslog_printf(char *fmt, ...)
{
    char line[SYSLOG_LINE_MAX];
    va_list args;

    va_start(args, fmt);
    vsnprintf(line, SYSLOG_LINE_MAX, fmt, args);
    va_end(args);

    syslog_print(line);
}

void syslog_vprintf(char *fmt, va_list args)
{
    char line[SYSLOG_LINE_MAX];
    vsnprintf(line, SYSLOG_LINE_MAX, fmt, args);
    syslog_print(line);
}

void syslog_dump_bytes(unsigned char *rdata, unsigned rlen) {
    unsigned i, j;
    char line[SYSLOG_LINE_MAX];
    for (i = 0; i < rlen / 16; i++) {
        for (j = 0; j < 16; j++) {
            sprintf(line + 3 * j, "%02x ", rdata[i * 16 + j]);
        }
        syslog_print(line);
    }
}

void syslog_dump(unsigned max)
{
    syslog_context_t *log = &consoleLog;
    uint64_t ts;
    char *start;
    char *end;
    char *line;
    unsigned count = 0;

    line = SYSLOG_MALLOC(SYSLOG_LINE_MAX);
    SYSLOG_CRITICAL_ENTRY(log->lock);
    while ((log->tail_idx != log->head_idx) && (count < max)) {
        log->tail_idx++;
        if (log->tail_idx == SYSLOG_MAX_LINES) {
            log->tail_idx = 0;
        }
        ts = log->ts_data[log->tail_idx];
        start = &log->log_data[log->tail_idx * SYSLOG_LINE_MAX];
        strncpy(line, start, SYSLOG_LINE_MAX);
        SYSLOG_CRITICAL_EXIT(log->lock);
        end = line + strlen(line) - 1;
        while (isspace((int)(*end))) {
            *end = 0;
            end--;
        }
        uint64_t ts_sec, ts_ms;
        ts_sec = ts / 1000;
        ts_ms = ts - ts_sec * 1000;
        printf("[%6lu.%03lu] ", (unsigned long)ts_sec,
            (unsigned long)ts_ms);
        puts(line);
        count++;
        SYSLOG_CRITICAL_ENTRY(log->lock);
    }
    SYSLOG_CRITICAL_EXIT(log->lock);
    SYSLOG_FREE(line);
}

char *syslog_next(char *ts, size_t tsMax, char *line, size_t lineMax)
{
    syslog_context_t *log = &consoleLog;
    char *logLine;
    uint64_t u64ts;
    char *end;

    SYSLOG_CRITICAL_ENTRY(log->lock);
    if (log->tail_idx != log->head_idx) {
        log->tail_idx++;
        if (log->tail_idx == SYSLOG_MAX_LINES) {
            log->tail_idx = 0;
        }
        logLine = &log->log_data[log->tail_idx * SYSLOG_LINE_MAX];
        strncpy(line, logLine, lineMax); line[lineMax-1] = '\0';
        u64ts = log->ts_data[log->tail_idx];
    } else {
        line = NULL; ts = NULL;
    }
    SYSLOG_CRITICAL_EXIT(log->lock);

    if (line) {
        end = line + strlen(line) - 1;
        while (isspace((int)(*end))) {
            *end = 0;
            end--;
        }
    }

    if (ts) {
        uint64_t ts_sec, ts_ms;
        ts_sec = u64ts / 1000;
        ts_ms = u64ts - ts_sec * 1000;
        snprintf(ts, tsMax, "[%6lu.%03lu] ", (unsigned long)ts_sec,
            (unsigned long)ts_ms);
        ts[tsMax-1] = '\0';
    }

    return(line);
}
