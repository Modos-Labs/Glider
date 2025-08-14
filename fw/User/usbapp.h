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
#pragma once

#define USBCMD_RESET        0x00
#define USBCMD_POWERDOWN    0x01
#define USBCMD_POWERUP      0x02
#define USBCMD_SETINPUT     0x03
#define USBCMD_REDRAW       0x04
#define USBCMD_SETMODE      0x05
#define USBCMD_NUKE         0x06
#define USBCMD_USBBOOT      0x07
#define USBCMD_RECV         0x08

#define USBRET_GENERALFAIL  0x00
#define USBRET_CHKSUMFAIL   0x01
#define USBRET_SUCCESS      0x55

void usbapp_term_out(char data, void *usr);
int usbapp_term_in(int mode, void *usr);
portTASK_FUNCTION(usb_device_task, pvParameters);
