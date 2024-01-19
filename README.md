# Glider

Open source E-ink monitor with an emphasis on low latency.

Note: This repo only contains the hardware design, the gateware running on the FPGA is our open-source Caster EPDC design, which could be found at [github.com/Modos-Labs/Caster](https://github.com/Modos-Labs/Caster/)

Note: The design is still working in progress. There are a few known bugs that will be fixed in the next revision.

## Feature

- FPGA for field upgradable EPD controller design
- Supports various different Eink/ DES panels
- Up to 1.6GB/s DDR3-800 buffer memory
- Up to 224 MPixel/s DisplayPort 1.2 input (2200x1650 @ 60Hz)

## License

The hardware design is released under the CERN Open Source Hardware License strongly-reciprocal variant, CERN-OHL-S. A copy of the license is provided in the source repository. Additionally, user guide of the license is provided on ohwr.org.
