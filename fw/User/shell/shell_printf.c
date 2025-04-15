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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "shell_printf.h"
#include "term.h"
#include "shell.h"

/* Use lightweight printf */
//#include "printf.h"

int shell_vprintf(shell_context_t *ctx, const char *fmt, va_list ap)
{
    va_list va;
    char *str;
    va = ap;
    int len = vsnprintf(NULL, 0, fmt, va);
    va = ap;
    str = SHELL_MALLOC(len + 1);
    vsnprintf(str, len + 1, fmt, va);
    term_putstr(&ctx->t, str, len);
    SHELL_FREE(str);
    return(len);
}

int shell_printf(shell_context_t *ctx, const char* fmt, ...)
{
    va_list va;
    char *str;
    va_start(va, fmt);
    int len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);
    va_start(va, fmt);
    str = SHELL_MALLOC(len + 1);
    vsnprintf(str, len + 1, fmt, va);
    va_end(va);
    term_putstr(&ctx->t, str, len);
    SHELL_FREE(str);
    return(len);
}
