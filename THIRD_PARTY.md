# Third-party source and licenses

- [Brogue CE](https://github.com/tmewett/BrogueCE), v1.15.1 commit `1ba4240b7a928ddf0ffb772717bf1d433cd63804`: selected original C source and headers in `deps/BrogueCE/`, unchanged. AGPL-3.0 text is preserved at `deps/BrogueCE/LICENSE.txt` and `LICENSE`.
- [PicoPlus-devel/pico_shared](https://github.com/PicoPlus-devel/pico_shared), base commit `df683bf00ff95e1ceda88f132bbf72f8cc7cadb0`: selected public source in `deps/pico_shared/`, including the HSTX and FatFs drivers. Its GPL-3.0 license is at `deps/pico_shared/LICENSE`; the `pico_fatfs` BSD-style license is at `deps/pico_shared/drivers/pico_fatfs/LICENSE`. The HSTX driver is adapted at CMake configure time solely to call the Brogue scanline renderer; the vendored file itself is untouched.
- The port runtime in `platform/wave1/` and Loader helper were adapted from the related public [ClassicIF](https://github.com/jackyangantelope/ClassicIF) source at commit `924ab3687a3f235d6a69a935ac093c3f47882126`, with the USB key-event queue adapted from the original Brogue port. The shared runtime notice is at `LICENSES/jack-green-ports-BSD-3-Clause.txt`.
- The 6×8 text font in `platform/brogue_font_6x8.h` retains its Expat notice at `LICENSES/brogue-font-Expat.txt`.
- Pico SDK and Pico-PIO-USB are external build dependencies, not bundled. Follow their upstream notices and licenses.

This repository contains no Brogue graphical tiles, ROMs, save games, or prebuilt UF2.

