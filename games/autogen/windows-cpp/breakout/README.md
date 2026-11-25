# Breakout

Breakout from `breakout.xml`, written as a small program of its own on SFML 3.

| File | What |
|---|---|
| `source/breakout.h` | its numbers, its things and its class |
| `source/breakout.cpp` | its rules: where things start, the keys, and a frame |
| `source/main.cpp` | the window: keys and the mouse in, a step of the time since the last frame, the picture and the sounds out |
| `assets/tuffy.ttf` | the font, copied beside the program |
| `LICENSE` | the licence (MIT) |
| `CMakeLists.txt` | the build; it uses installed SFML 3, or downloads and builds it |
| `.gitignore` | leaves out `build/` and `output/` |

## Build and play

On Windows with Visual Studio and vcpkg (`vcpkg install sfml`), from this folder:

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
output\Release\breakout.exe
```

Without vcpkg, leave out the toolchain file: SFML is downloaded and built with the game.
