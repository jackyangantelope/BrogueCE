# Brogue CE for RP2350 Fruit Jam

This is a source-only, text-mode port of [Brogue Community Edition](https://github.com/tmewett/BrogueCE) to the publicly available Adafruit Fruit Jam (RP2350B). It builds a Loader-partition UF2 named `picoBrogueCE.uf2`. No game data or binary release is included.

## Status

The underlying native-text port (`0.1.1-native-text`) passed basic tests on a separate RP2350B development setup: game start, common USB keyboard actions, and basic save/load. Those results do **not** constitute Fruit Jam validation. This public-board adaptation compiles and passes UF2 layout validation, but has **not yet been tested on Fruit Jam hardware**. In particular, its 640×480 scanline renderer, PSRAM timing, USB host input, SD persistence, and Loader integration need device testing. The linker reports about 95% of on-chip RAM in use, leaving limited headroom.

The port intentionally has no music or sound effects. It renders a 100×34 character grid at 640×480, with an optional 40×30 zoom view. F1 toggles zoom; F2/F3 move horizontally and F4/F5 move vertically while zoomed. Keyboard arrows map to Brogue's `h/j/k/l`. The Fruit Jam build currently uses a USB keyboard; controller input is not validated or advertised. Saves, scores and recordings use `/SAVES/BROGUECE/` on the SD card. There is no battery-backed clock: timestamps combine a random boot epoch with uptime and are not real-world time.

Do not flash this source candidate to a device holding valuable data without a backup. No UF2 is published here pending Fruit Jam testing.

## Build

Requirements: Pico SDK 2.3.0 or newer, the RP2350 ARM GCC toolchain, CMake, Ninja, Python, and a Pico-PIO-USB checkout compatible with the SDK. The selected Brogue CE and `pico_shared` source subsets are already in `deps/`; no submodules are required.

On Windows PowerShell, from the repository root:

```powershell
./scripts/build-release.ps1 `
  -PicoSdkPath 'C:\path\to\pico-sdk' `
  -PicoPioUsbPath 'C:\path\to\Pico-PIO-USB' `
  -ToolchainPath 'C:\path\to\arm-toolchain' `
  -NinjaPath 'C:\path\to\ninja.exe' `
  -PioasmDir 'C:\path\to\pioasm' `
  -PicotoolDir 'C:\path\to\picotool' `
  -TinyUsbPath 'C:\path\to\pico-sdk\lib\tinyusb'
python ./scripts/validate-uf2-layout.py ./build/fruit-jam-release/picoBrogueCE.uf2
```

The default application partition starts at `0x10080000`; the generated UF2 is intended for a compatible resident Loader, not as a standalone boot image. Build outputs are ignored by Git.

## Source and changes

The game engine is selected, unmodified source from [Brogue CE v1.15.1](https://github.com/tmewett/BrogueCE/tree/1ba4240b7a928ddf0ffb772717bf1d433cd63804), commit `1ba4240b7a928ddf0ffb772717bf1d433cd63804`, in `deps/BrogueCE/`. Its AGPL-3.0 license is at `deps/BrogueCE/LICENSE.txt` and the repository root. Optional CC BY-SA graphical tiles are not included.

The port-specific files are:

- `firmware/brogue_entry.c`: enters the upstream game loop on a reserved PSRAM stack.
- `firmware/playable_platform.cpp`: initializes video, keyboard, PSRAM and SD; maps keyboard events and submits the character grid.
- `platform/brogue_embedded_console.c` and `brogue_embedded_contract.h`: implement the upstream console boundary without changing engine source.
- `platform/brogue_video.cpp`, `brogue_video.h`, `brogue_font_6x8.h`, and `cmake/BrogueVideo.cmake`: render native text through a guarded HSTX scanline hook.
- `platform/brogue_psram_heap.cpp/.h` and `cmake/sections_psram.incl`: reserve PSRAM for large engine data, heap and stack.
- `platform/brogue_storage.cpp`, `fatfs_dirent.cpp`, `dirent.h`, and `brogue_time.cpp`: bridge files, directory enumeration and time to SD/FatFs.
- `platform/wave1/`, `cmake/LoaderPartition.cmake`, and `scripts/`: adapt the shared Fruit Jam runtime, Loader layout and reproducible build checks.

Third-party provenance and licenses are recorded in [THIRD_PARTY.md](THIRD_PARTY.md). The release tooling used to audit this drop is kept local and is not part of the published tree.

