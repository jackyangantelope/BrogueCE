cmake_minimum_required(VERSION 3.20)
set(PICO_BOARD adafruit_fruit_jam CACHE STRING "Adafruit Fruit Jam" FORCE)
set(PICO_PSRAM_CS_PIN 47 CACHE STRING "Fruit Jam PSRAM chip select" FORCE)
set(PICO_PSRAM_SIZE_BYTES 0x800000 CACHE STRING "Fruit Jam PSRAM size" FORCE)
set(PICO_DEFAULT_UART 0 CACHE STRING "Diagnostic UART" FORCE)
set(PICO_DEFAULT_UART_TX_PIN 44 CACHE STRING "Diagnostic UART TX" FORCE)
set(PICO_DEFAULT_UART_RX_PIN 45 CACHE STRING "Diagnostic UART RX" FORCE)
set(PICO_FLASH_SPI_CLKDIV 10 CACHE STRING "Conservative XIP boot divider" FORCE)
if(NOT DEFINED PICO_SDK_PATH)
    message(FATAL_ERROR "Set PICO_SDK_PATH to Pico SDK 2.3.0 or newer")
endif()
set(PICO_SHARED_PATH "${CMAKE_CURRENT_LIST_DIR}/deps/pico_shared" CACHE PATH "pico_shared source")
if(NOT DEFINED PICO_PIO_USB_PATH)
    message(FATAL_ERROR "Set PICO_PIO_USB_PATH to a Pico-PIO-USB checkout")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
project(picoBrogueCE C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(BROGUE_ROOT "${CMAKE_CURRENT_LIST_DIR}/deps/BrogueCE")
set(PORT_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(BROGUE_INCLUDE_DIRS "${BROGUE_ROOT}/src/brogue" "${BROGUE_ROOT}/src/platform"
    "${BROGUE_ROOT}/src/variants" "${PORT_ROOT}/platform")
file(GLOB BROGUE_CORE_SOURCES CONFIGURE_DEPENDS "${BROGUE_ROOT}/src/brogue/*.c")
file(GLOB BROGUE_VARIANT_SOURCES CONFIGURE_DEPENDS "${BROGUE_ROOT}/src/variants/*.c")
add_library(brogue_ce_core STATIC ${BROGUE_CORE_SOURCES} ${BROGUE_VARIANT_SOURCES}
    "${BROGUE_ROOT}/src/platform/platformdependent.c"
    "${PORT_ROOT}/platform/brogue_embedded_console.c")
target_include_directories(brogue_ce_core PUBLIC ${BROGUE_INCLUDE_DIRS})
target_compile_definitions(brogue_ce_core PRIVATE BROGUE_EXTRA_VERSION=""
    malloc=brogue_psram_malloc calloc=brogue_psram_calloc
    realloc=brogue_psram_realloc free=brogue_psram_free)
target_compile_options(brogue_ce_core PRIVATE -O2 -ffunction-sections -fdata-sections
    -Wno-format -Wno-unused-result -Wno-stringop-truncation
    -include "${PORT_ROOT}/platform/brogue_psram_heap.h")

include("${PORT_ROOT}/cmake/BrogueVideo.cmake")
add_subdirectory("${PORT_ROOT}/platform/wave1" "${CMAKE_BINARY_DIR}/wave1")
target_compile_definitions(jack_wave1_runtime PRIVATE JACK_WAVE1_USB_NONBLOCKING=1
    JACK_WAVE1_SYS_CLOCK_KHZ=252000u JACK_WAVE1_FLASH_MAX_HZ=42000000u
    JACK_WAVE1_PSRAM_MAX_HZ=96000000u)
target_link_libraries(jack_wave1_runtime PRIVATE hardware_pll hardware_psram)

option(BUILD_FOR_BOOTLOADER "Link to the resident loader app partition" ON)
add_executable(picoBrogueCE
    "${PORT_ROOT}/firmware/playable_platform.cpp"
    "${PORT_ROOT}/firmware/brogue_entry.c"
    "${PORT_ROOT}/platform/fatfs_dirent.cpp"
    "${PORT_ROOT}/platform/brogue_storage.cpp"
    "${PORT_ROOT}/platform/brogue_time.cpp"
    "${PORT_ROOT}/platform/brogue_video.cpp"
    "${PORT_ROOT}/platform/brogue_psram_heap.cpp")
pico_set_program_name(picoBrogueCE "picoBrogueCE")
pico_set_program_version(picoBrogueCE "0.1.1-fruit-jam-source-candidate")
pico_enable_stdio_uart(picoBrogueCE 1)
pico_enable_stdio_usb(picoBrogueCE 0)
target_include_directories(picoBrogueCE PRIVATE ${BROGUE_INCLUDE_DIRS})
target_compile_definitions(picoBrogueCE PRIVATE HW_CONFIG=8 PICO_STACK_SIZE=4096
    PICO_FLASH_SPI_CLKDIV=10 PICO_PSRAM_CS_PIN=47 PICO_PSRAM_SIZE_BYTES=0x800000)
target_link_libraries(picoBrogueCE PRIVATE pico_stdlib hardware_psram pico_rand
    brogue_ce_core jack_wave1_runtime)
target_link_options(picoBrogueCE PRIVATE -Wl,--gc-sections -Wl,--print-memory-usage)
if(BUILD_FOR_BOOTLOADER)
    include("${PORT_ROOT}/cmake/LoaderPartition.cmake")
    brogue_link_for_loader(picoBrogueCE)
endif()
pico_add_extra_outputs(picoBrogueCE)
