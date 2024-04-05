# Install python3 HID package https://pypi.org/project/hid/
import hid
import struct

vid = 0xcafe
pid = 0x4004

dev = hid.Device(vid, pid)

if not dev:
    print("Unable to open device")
    exit()

cmd = 0x00
param = 0x00
x0 = 0
y0 = 0
x1 = 1600 - 1
y1 = 1200 - 1
pid = 0
chksum = 0

str_out = struct.pack('>h>h>h>h>h>h>h>h', cmd, param, x0, y0, x1, y1, pid, chksum)
dev.write(str_out)
str_in = dev.read(1)
print("Received from HID Device:", str_in, '\n')
