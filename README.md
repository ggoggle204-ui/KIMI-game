# Sky Circuit Precision Driver - PS3 Port

A C/C++ port of the 2D canvas driving game (`25void.html`) targeting the
PlayStation 3 Super Slim (CECH-40xx), built with the free PSL1GHT homebrew
toolchain. All rendering is done in software at 720p and blitted through tiny3d;
audio is synthesized in software (engine hum, tyre screech, beeps, explosion)
and fed to libaudio at 48 kHz.

## Layout

    src/game.{h,cpp}        portable game core (physics, tracks, burnout, menus)
    src/raster.{h,cpp}      software framebuffer: lines, tris, circles, text
    src/font.h              tiny 3x5 bitmap font
    src/audio_synth.{h,cpp} SFX synthesizer (portable)
    src/ps3/main_ps3.cpp    PSL1GHT platform layer (tiny3d + libio pad + libaudio)
    src/sdl/main_sdl.cpp    SDL2 desktop build for quick testing
    tools/headless_test.cpp logic smoke test (no display needed)

## Controls (Sixaxis / DualShock 3)

    D-Pad Left/Right  steer          CROSS    gas / confirm
    SQUARE            reverse/brake  CIRCLE   handbrake
    START             pause          SELECT   back / respawn menu

In pause menu: UP/DOWN + CROSS (toggle road width = second option).
Burnout: hold CROSS + SQUARE together, release one pedal to launch.

## Building for PS3

1. Install the ps3toolchain (provides ppu-g++/spu-g++), PSL1GHT, and
   ps3libraries (needs at least: tiny3d, libaudio (Estwald), libio, libsysutil,
   gcm/reality/rsx). Set PS3DEV/PSL1GHT env vars as usual.
2. `make -f Makefile.ps3`
3. Package: `make pkg` (or use make_self_npdrm + a pkg tool) and install the
   .pkg on the console.

### Super Slim note
The Super Slim cannot run custom firmware - it runs homebrew through
**HEN (Hybrid Firmware, 4.84+)**. Install HFW + HEN, enable HEN, then install
the signed homebrew package via Package Manager / Install Package Files.

If your tiny3d/libaudio headers differ slightly from the calls in
`src/ps3/main_ps3.cpp` (these libraries have several fork variants), adjust the
few affected lines - the game core does not depend on them.

## Building for desktop (test)

    make            # requires SDL2 (sdl2-config)
    ./skycircuit

or, without SDL:

    g++ -O2 -std=c++11 -Isrc src/*.cpp tools/headless_test.cpp -o headless_test
    ./headless_test && headless.ppm produced
