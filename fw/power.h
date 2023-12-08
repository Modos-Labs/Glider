//
// Copyright 2023 Wenting Zhang <zephray@outlook.com>
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
#pragma once

#if defined(POWER_TPS65185)
#include "tps65185.h"

#define power_init()        tps_init()
#define power_enable(x)     tps_enable(x)
#define power_set_vcom(x)   tps_set_vcom(x)

#elif defined(POWER_MAX17135)
#include "max17135.h"

#define power_init()        max_init()
#define power_enable(x)     max_enable(x)
#define power_set_vcom(x)   max_set_vcom(x)

#elif defined(POWER_GPIO)

void power_init(void);
void power_enable(bool en);
void power_set_vcom(uint16_t vcom);

#endif
