if(NOT TARGET jack_wave1_runtime)
    if(NOT DEFINED PICO_SHARED_PATH)
        message(FATAL_ERROR "PICO_SHARED_PATH is required")
    endif()
    if(NOT DEFINED PICO_PIO_USB_PATH)
        message(FATAL_ERROR "PICO_PIO_USB_PATH is required")
    endif()

    set(WAVE1_DIR "${CMAKE_CURRENT_LIST_DIR}")
    if(NOT EXISTS "${PICO_SHARED_PATH}/drivers/pico_hstx/hstx.c")
        message(FATAL_ERROR "pico_shared checkout not found: ${PICO_SHARED_PATH}")
    endif()

    add_subdirectory("${PICO_SHARED_PATH}/drivers/pico_fatfs"
                     "${CMAKE_BINARY_DIR}/pico_fatfs")

    if(NOT TARGET pico_hstx)
        add_subdirectory("${PICO_SHARED_PATH}/drivers/pico_hstx"
                         "${CMAKE_BINARY_DIR}/pico_hstx")
    endif()

    set(SAVED_TINYUSB_TOP "${TOP}")
    set(TOP "${PICO_SDK_PATH}/lib/tinyusb")
    check_and_add_pico_pio_usb_support()
    set(TOP "${SAVED_TINYUSB_TOP}")
    if(NOT TARGET tinyusb_pico_pio_usb)
        message(FATAL_ERROR "Pico-PIO-USB support could not be configured")
    endif()

    add_library(jack_wave1_runtime STATIC
        "${WAVE1_DIR}/jack_wave1_runtime.cpp"
        "${WAVE1_DIR}/jack_wave1_usb_fruit_jam.cpp"
        "${PICO_SHARED_PATH}/nespad.cpp")
    target_include_directories(jack_wave1_runtime PUBLIC
        "${WAVE1_DIR}"
        "${PICO_SHARED_PATH}"
        "${PICO_SHARED_PATH}/drivers/pico_fatfs"
        "${PICO_SHARED_PATH}/drivers/pico_fatfs/fatfs")
    target_compile_definitions(jack_wave1_runtime PUBLIC
        HW_CONFIG=8
        SDCARD_PIN_CS=39
        SDCARD_PIN_SCK=34
        SDCARD_PIN_MOSI=35
        SDCARD_PIN_MISO=36
        SDCARD_PIO=pio0
        SDCARD_SPI=spi0
        GPIOHSTXD0=15
        GPIOHSTXD1=17
        GPIOHSTXD2=19
        GPIOHSTXCK=13
        GPIOHSTXINVERTED=1
        CFG_TUH_RPI_PIO_USB=1
        PIO_USB_USE_PIO=2
        PICO_DEFAULT_PIO_USB_DP_PIN=1)
    target_compile_options(jack_wave1_runtime PRIVATE
        -Wall -Wextra -Wno-unused-parameter)
    target_link_libraries(jack_wave1_runtime PUBLIC
        pico_stdlib
        pico_multicore
        hardware_clocks
        hardware_dma
        hardware_pio
        hardware_spi
        hardware_timer
        hardware_vreg
        pico_fatfs
        pico_hstx
        tinyusb_host
        tinyusb_pico_pio_usb)
endif()
