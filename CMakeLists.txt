cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(rp2040_tester C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(rp2040_tester
    src/main.c
    src/port_drv.c
    src/adc_drv.c
    src/measure_res.c
    src/measure_cap.c
    src/measure_semi.c
    src/ssd1306.c
    src/ui.c
)

target_include_directories(rp2040_tester PRIVATE src)

target_link_libraries(rp2040_tester
    pico_stdlib
    hardware_adc
    hardware_i2c
    hardware_gpio
)

pico_enable_stdio_usb(rp2040_tester 1)
pico_enable_stdio_uart(rp2040_tester 0)
pico_add_extra_outputs(rp2040_tester)
