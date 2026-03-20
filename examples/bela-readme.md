# Sushi on Bela Gem Multi

This document explains how to build `libsushi` for the **Bela Gem Multi** and
integrate it into a Bela project using the Reactive frontend.

---

## Overview

The Bela Gem Multi exposes 10 analogue inputs and 10 analogue outputs on a
**PocketBeagle 2** (Texas Instruments AM6232/AM6254 SoC). It runs
**Xenomai 4 with the EVL core** — not Xenomai 3 / Cobalt, and not
Elk Audio OS.

Sushi runs on it via the **Reactive frontend**: Sushi has no audio thread of
its own; instead, Bela's `render()` callback drives `process_audio()` directly
each block. See `bela_gem_multi_host.cpp` in this directory for the full
integration example.

Key points:

- **RASPA is not used** — RASPA is Elk Audio OS's proprietary SPI audio driver
  and has nothing to do with Bela.
- **Twine is required** — `twine::current_rt_time()` reads the EVL monotonic
  clock inside `render()`. Twine is a git submodule of this repository.
- The compile-time buffer size **must match** the Bela block size you select
  in the Bela IDE (default: 64 frames).

---

## Prerequisites (both methods)

### 1. Initialise the Twine submodule

```bash
git submodule update --init twine
```

Twine is Elk Audio's real-time threading library. It is built automatically by
Sushi's CMake when `SUSHI_BUILD_TWINE=ON`.

### 2. Initialise third-party submodules (optional but recommended)

Only needed if you want VST3 support:

```bash
git submodule update --init --recursive third-party/vst3sdk
```

---

## Method A — Building directly on the Bela Gem Multi

SSH into the board, clone the repository, and build natively. The board is
`aarch64` Linux with EVL headers already present.

### Install build dependencies on the board

```bash
sudo apt update
sudo apt install cmake ninja-build build-essential libasound2-dev libevl-dev
```

> `libevl-dev` provides the EVL headers and `libevl` needed by Twine.

### Clone and build

```bash
git clone <this-repo-url> sushi
cd sushi
git submodule update --init twine

mkdir build && cd build

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUSHI_BUILD_STANDALONE_APP=OFF \
  -DSUSHI_BUILD_TWINE=ON \
  -DTWINE_WITH_EVL=ON \
  -DSUSHI_WITH_RASPA=OFF \
  -DSUSHI_WITH_JACK=OFF \
  -DSUSHI_WITH_ALSA_MIDI=ON \
  -DSUSHI_WITH_RT_MIDI=OFF \
  -DSUSHI_WITH_VST3=ON \
  -DSUSHI_WITH_LV2=OFF \
  -DSUSHI_WITH_UNIT_TESTS=OFF \
  -DSUSHI_AUDIO_BUFFER_SIZE=64

cmake --build . --target sushi_library -j$(nproc)
```

> **`SUSHI_AUDIO_BUFFER_SIZE=64` must match the block size you select in the
> Bela IDE.** If you change the Bela block size at runtime you must recompile
> `libsushi` with the matching value.

After a successful build, `libsushi.so` (or `libsushi.a`) is in `build/`.
The public headers are under `include/sushi/`.

---

## Method B — Cross-compiling from a Linux desktop

Cross-compilation requires:

1. An `aarch64-linux-gnu` toolchain
2. A sysroot containing the Bela board's libraries and headers

### Install the cross-toolchain

```bash
# Debian / Ubuntu
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu cmake ninja-build
```

### Obtain a sysroot

The easiest approach is to copy the relevant directories from a running board:

```bash
rsync -av --rsync-path="sudo rsync" \
  root@<bela-ip>:/usr/include \
  root@<bela-ip>:/usr/lib/aarch64-linux-gnu \
  root@<bela-ip>:/usr/lib/evl \
  ~/bela-sysroot/usr/
```

You need at minimum:
- ALSA headers and library (`libasound2-dev` equivalent)
- EVL headers and `libevl` (`libevl-dev` equivalent)
- Standard C++ runtime for aarch64

### Create a CMake toolchain file

Save the following as `cmake/bela-aarch64-toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_SYSROOT /path/to/your/bela-sysroot)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

Replace `/path/to/your/bela-sysroot` with the actual path.

### Configure and build

```bash
mkdir build-bela && cd build-bela

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/bela-aarch64-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DSUSHI_BUILD_STANDALONE_APP=OFF \
  -DSUSHI_BUILD_TWINE=ON \
  -DTWINE_WITH_EVL=ON \
  -DSUSHI_WITH_RASPA=OFF \
  -DSUSHI_WITH_JACK=OFF \
  -DSUSHI_WITH_ALSA_MIDI=ON \
  -DSUSHI_WITH_RT_MIDI=OFF \
  -DSUSHI_WITH_VST3=ON \
  -DSUSHI_WITH_LV2=OFF \
  -DSUSHI_WITH_UNIT_TESTS=OFF \
  -DSUSHI_AUDIO_BUFFER_SIZE=64

cmake --build . --target sushi_library -j$(nproc)
```

When `CMAKE_CROSSCOMPILING` is detected by CMake, Sushi's `CMakeLists.txt`
automatically disables unit tests and enables ALSA MIDI, which is correct for
this target.

### Deploy to the board

```bash
scp build-bela/libsushi.so root@<bela-ip>:/usr/local/lib/
scp -r include/sushi            root@<bela-ip>:/usr/local/include/
```

---

## Integrating with your Bela project

### Drop-in usage

Copy `bela_gem_multi_host.cpp` into your Bela project folder. The Bela IDE
compiles every `.cpp` file in the project directory.

### Tell Bela's build system where to find Sushi

In the Bela IDE, open **Project Settings → Custom compiler flags** and add:

```
-I/usr/local/include -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=64
```

Under **Custom linker flags**:

```
-L/usr/local/lib -lsushi -lstdc++ -lpthread -levl
```

Or, if using a custom Makefile:

```makefile
CPPFLAGS += -I/usr/local/include -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=64
LDFLAGS  += -L/usr/local/lib -lsushi -lstdc++ -lpthread -levl
```

### Loading a Sushi configuration

By default `bela_gem_multi_host.cpp` starts with `ConfigurationSource::NONE`
(no tracks, silence). To load a real patch, change the `setup()` function:

```cpp
options.config_source   = sushi::ConfigurationSource::FILE;
options.config_filename = "/path/to/your/config.json";
```

---

## CMake option reference

| Option | Bela value | Notes |
|---|---|---|
| `SUSHI_BUILD_STANDALONE_APP` | `OFF` | Building a library, not the `sushi` executable |
| `SUSHI_BUILD_TWINE` | `ON` | Build the bundled Twine submodule |
| `TWINE_WITH_EVL` | `ON` | Bela Gem Multi uses Xenomai 4 / EVL |
| `TWINE_WITH_XENOMAI` | `OFF` | Xenomai 3 / Cobalt is for the original Bela (BeagleBone), not Gem |
| `SUSHI_WITH_RASPA` | `OFF` | RASPA is for Elk Audio OS only |
| `SUSHI_WITH_JACK` | `OFF` | Not available on Bela |
| `SUSHI_WITH_ALSA_MIDI` | `ON` | MIDI over USB uses ALSA on Bela |
| `SUSHI_WITH_RT_MIDI` | `OFF` | Mutually exclusive with ALSA MIDI |
| `SUSHI_WITH_UNIT_TESTS` | `OFF` | Tests cannot run on the target |
| `SUSHI_AUDIO_BUFFER_SIZE` | `64` | **Must match Bela IDE block size** |

---

## Troubleshooting

**Assertion failure at startup: `Bela block size must match SUSHI_CUSTOM_AUDIO_CHUNK_SIZE`**

The block size selected in the Bela IDE differs from `SUSHI_AUDIO_BUFFER_SIZE`
used at compile time. Recompile `libsushi` with the matching value, or change
the block size in the IDE.

**`twine/twine.h: No such file or directory`**

The Twine submodule is not initialised. Run:

```bash
git submodule update --init twine
```

**`libevl not found` during CMake configuration**

Install `libevl-dev` on the board, or add the sysroot path containing EVL
headers to your toolchain file.

**Sushi starts but produces silence**

`ConfigurationSource::NONE` is the default — no tracks are loaded. Point
`config_filename` at a valid Sushi JSON config and set
`config_source = ConfigurationSource::FILE`.
