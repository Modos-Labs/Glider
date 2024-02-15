# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/wenting/pico/pico-sdk")
set(PICO_EXTRAS_PATH "/home/wenting/pico/pico-extras")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

project(fw C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(fw
        bitstream.c
        caster.c
        edid.c
        fpga.c
        fusb302.c
        fw.c
        max17135.c
        power.c
        ptn3460.c
        tcpm_driver.c
        tps65185.c
        usb_descriptors.c
        usb_mux.c
        usb_pd_driver.c
        usb_pd_policy.c
        usb_pd_protocol.c
        usbapp.c
        utils.c
        )

pico_set_program_name(fw "fw")
pico_set_program_version(fw "0.1")

pico_enable_stdio_uart(fw 0)
pico_enable_stdio_usb(fw 0)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(fw PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# Add the standard library to the build
target_link_libraries(fw pico_stdlib hardware_sleep tinyusb_device tinyusb_board)

# Add any user requested libraries
target_link_libraries(fw
        hardware_spi
        hardware_dma
        hardware_i2c
        hardware_pwm
        pico_unique_id
        pico_i2c_slave 
        )

pico_add_extra_outputs(fw)

