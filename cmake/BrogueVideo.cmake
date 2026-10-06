set(BROGUE_HSTX_DIR "${PICO_SHARED_PATH}/drivers/pico_hstx")
file(READ "${BROGUE_HSTX_DIR}/hstx.c" BROGUE_HSTX_SOURCE)
set(BROGUE_HSTX_ANCHOR "                __dmb();")
string(REPLACE "${BROGUE_HSTX_ANCHOR}" "" BROGUE_HSTX_WITHOUT_ANCHOR "${BROGUE_HSTX_SOURCE}")
string(LENGTH "${BROGUE_HSTX_SOURCE}" BROGUE_HSTX_LENGTH)
string(LENGTH "${BROGUE_HSTX_WITHOUT_ANCHOR}" BROGUE_HSTX_REMAINING_LENGTH)
string(LENGTH "${BROGUE_HSTX_ANCHOR}" BROGUE_HSTX_ANCHOR_LENGTH)
math(EXPR BROGUE_HSTX_REMOVED_LENGTH "${BROGUE_HSTX_LENGTH} - ${BROGUE_HSTX_REMAINING_LENGTH}")
if(NOT BROGUE_HSTX_REMOVED_LENGTH EQUAL BROGUE_HSTX_ANCHOR_LENGTH)
    message(FATAL_ERROR "Brogue HSTX scanline hook no longer matches the driver; review upstream changes")
endif()
string(REPLACE "#include \"hstx.h\"" "#include \"hstx.h\"\n#include \"brogue_video.h\""
       BROGUE_HSTX_SOURCE "${BROGUE_HSTX_SOURCE}")
string(REPLACE "${BROGUE_HSTX_ANCHOR}" "${BROGUE_HSTX_ANCHOR}
                const uint32_t render_start = time_us_32();
                if (brogue_video_render_scanline(p, (unsigned)load_line)) {
                    brogue_video_record_scanline(time_us_32() - render_start,
                                                 v_scanline != (unsigned)last_line);
                    continue;
                }" BROGUE_HSTX_SOURCE "${BROGUE_HSTX_SOURCE}")
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/brogue_hstx.c"
     CONTENT "${BROGUE_HSTX_SOURCE}" @ONLY)
add_library(pico_hstx INTERFACE)
target_sources(pico_hstx INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/brogue_hstx.c")
target_include_directories(pico_hstx INTERFACE "${BROGUE_HSTX_DIR}" "${PORT_ROOT}/platform")
target_link_libraries(pico_hstx INTERFACE pico_stdlib pico_multicore hardware_dma pico_sync)
