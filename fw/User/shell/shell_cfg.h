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
#pragma once

#include "platform.h"

#define SHELL_MAX_ARGS          10
#define SHELL_WELCOMEMSG        "Modos Grimoire\n" \
                                "Version: %s (%s %s)\n"
#define SHELL_PROMPT            "# "
#define SHELL_ERRMSG            "Invalid command, type 'help' for help\n"
#define SHELL_MAX_LINE_LEN      79
#define SHELL_COLUMNS           80
#define SHELL_LINES             24
#define SHELL_MAX_HISTORIES     50
#define SHELL_MALLOC            pvPortMalloc
#define SHELL_FREE              vPortFree
// Does not provide calloc/ realloc
#define SHELL_CALLOC            
#define SHELL_REALLOC           
#define SHELL_STRDUP            shell_strdup
#define SHELL_STRNDUP           shell_strndup
