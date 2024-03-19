# Glider

Open source E-ink monitor with an emphasis on low latency.

Note: This repo only contains the hardware design, the gateware running on the FPGA is our open-source Caster EPDC design, which could be found at [github.com/Modos-Labs/Caster](https://github.com/Modos-Labs/Caster/)

![block-diagram](blockdiagram.svg)

## Feature

- Xilinx(R) Spartan-6 LX16 FPGA running Caster
- DDR3-800 framebuffer memory
- Type-C DisplayPort Alt-Mode video input with on-board PTN3460 DP-LVDS bridge or
- DVI (via microHDMI connector) video input with on-board ADV7611 decoder
- Epaper power supply with up to 1A peak current on +/-15V rail supporting large panels
- VCOM kick-back voltage measurement support
- On-board RaspberryPi(R) RP2040 microcontroller for USB communication and firmware upgrade
- Up to 133MP/s processing rate with dithering enabled, >200MP/s when disabled

## Components

This repo hosts the PCB design, firmware source code, and a reference 3D-printable case design.

### PCB

The PCB is designed with KiCAD 8.0. To get optimal results, use a 4-layer stack up with 1080 PP layers, but 2313 or 3313 are also acceptable.

There are 2 versions of the mainboard, one full version and a lite version. For now only the full version is being actively worked on. The lite version removes dedicated external decoders for DVI/ DP and removes bulk of the TypeC circuitry to lower the cost. The only video interface being a DVI port feeding directly into the FPGA.

### Firmware

The MCU manages house-keeping items as described below. It currently doesn’t use any operating system, but rather relying on a main-loop style operation.

- EPD Power Supply: The power supply provides common, source, and gate supply to the EPD panel. In addition to basic on/off switch, the MCU can also adjust the VCOM voltage, and measure the optimal VCOM voltage of the panel installed. The VCOM measurement is done by isolating the VCOM supply from the screen with keeping the gate supply, scan the screen with source = VCOM, and measure the kick-back voltage on VCOM.
- FPGA Bitstream Loading: The FPGA doesn’t have its own flash memory, the bitstream is pushed over SPI from the MCU to the FPGA upon powering up. In this way the FPGA bitstream can be easily bundled together with the MCU’s firmware and updated together.
- Type-C Negotiation: The on-board USB Type-C port can support video input in addition to powering the board using the USB-C DisplayPort Alt Mode. The MCU runs a USB-C PD stack to communicate this capability to the video source device over standard USB PD protocol. The MCU also controls the Type-C signal mux to remap the lanes to the correct place depending on the cable orientation.
- Video decoder initialization: The FPGA used on the board doesn’t have high speed deserializers to interface with common high speed video interface such as DisplayPort or DVI directly. Instead, dedicated video decoder chips are used on the board. They typically needs initialization before use, and the MCU takes care of this. In this specific design the DP decoder chip also handles AUX P/N line flipping based on the Type-C cable orientation.
- PC communication: One advantage of the Caster is that update modes and forced update/ clearing can be applied on a per-pixel basis. Software may utilize this to assign different modes to different windows or change update modes depending on the content on the fly. This is done by sending requests to the MCU over USB connection. The MCU runs TinyUSB and present itself as an HID device so it can forward messages between the hos PC and the FPGA.

### Case

A reference case design is provided. The case is 3D printable and designed with FreeCAD.

## Pixel Rate Reference

The input protocol and processing rate limits the screen model supported.

- Processing rate when dithering enabled: 133 MP/s
- Processing rate when dithering disabled: 240 MP/s
- Maximum pixel rate using DVI (with ADV7611): 165 MP/s
- Maximum pixel rate using DVI (direct deserializer): 105 MP/s
- Maximum pixel rate using DisplayPort (with PTN3460): 224 MP/s
- Maximum pixel rate using DisplayPort (with 7-series 6G SerDes): 720 MP/s
- Maximum pixel rate using MIPI (with 1.05Gbps LVDS): 230 MP/s

Common screen resolution peak pixel rate (with CVT-RBv2):

- 1024x758 (6.0") @ 85Hz: 74 MP/s
- 1448x1072 (6.0") @ 60Hz: 101 MP/s
- 1448x1072 (6.0") @ 85Hz: 145 MP/s
- 1600x1200 (13.3") @ 60Hz: 125 MP/s
- 1600x1200 (13.3") @ 85Hz: 178 MP/s
- 1872x1404 (10.3") @ 60Hz: 169 MP/s
- 1872x1404 (10.3") @ 85Hz: 243 MP/s
- 2200x1650 (13.3") @ 60Hz: 232 MP/s
- 2200x1650 (13.3") @ 85Hz: 333 MP/s
- 2560x1600 (12.0") @ 60Hz: 261 MP/s
- 2560x1600 (12.0") @ 85Hz: 374 MP/s
- 3200x1800 (25.3") @ 60Hz: 364 MP/s
- 3200x1800 (25.3") @ 85Hz: 522 MP/s

## License

The hardware design is released under the CERN Open Source Hardware License strongly-reciprocal variant, CERN-OHL-S. A copy of the license is provided in the source repository. Additionally, user guide of the license is provided on ohwr.org.

The firmware code is licensed under the MIT license with the following exceptions:

The USB PD library is derived from the Chromium OS project and the reclamier labs. The library is licensed under the BSD license.
