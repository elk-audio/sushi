# SUSHI
Headless plugin host for ELK Audio OS.

## Usage

See `sushi -h` for a complete list of options.
Common use cases are:

Test in offline mode with I/O from audio file:

    $ sushi -o -i input_file.wav -c config_file.json

Use JACK for realtime audio:

    $ sushi -j -c config_file.json

With JACK, sushi creates 8 virtual input and output ports that you can connect to other programs or system outputs.

## SUSHI macOS (experimental)
Since version 1.0, SUSHI can be built natively for macOS as a native binary with all the dependencies statically linked to it.

There is a new Portaudio frontend (selectable with the `-p | --portaudio` command-line option) to interface directly with Portaudio. As an alternative, Jack can be used if it is available on the system.

MIDI support is provided through RtMidi and can access directly CoreMidi devices.

LV2 support is currently not available for macOS, and VST2 support is partial (it can only load at the moment plugins compiled in a Posix way and not the traditional macOS bundles which are used by commercial plugins).

## Configuration file examples

See directory `example_configs` for the JSON-schema definition and some example configurations.
Configuration files are used for global host configs, track and plugins configuration, MIDI routing and mapping, events sequencing.

## Building Sushi
Sushi uses CMake as its build system. A generate script is also provided for convenient setup. Simply running `./generate` with no arguments in the root of Sushi will setup a build folder containing a Release configuration and a Debug configuration. CMake arguments can be passed through the generate script using the `--cmake-args` flag. Those arguments will then be added to both configurations.

Make sure that Sushi is cloned with the `--recursive` flag to fetch all required submodules for building. Alternatively run `git submodule update --init --recursive` after cloning.

Sushi requires a compiler with support for C++17 features. The recommended compiler is GCC version 10 or higher.

### Building for native Linux
As all options are on by default, to build Sushi for a regular, non Elk Powered Linux distribution, the Xenomai options need to be turned off. In addition the Vst 2.x SDK needs to be provided (unless VST2 support is turned off).

    $ ./generate --cmake-args="-DWITH_XENOMAI=off -DVST2_SDK_PATH=/home/elk/sdks/vstsdk2.4" -b

If the Vst 2.4 SDK is not available, Sushi can still be built without Vst 2.x support using the command below:

    $ ./generate --cmake-args="-DWITH_XENOMAI=off -DWITH_VST2=off" -b

It is also possible to skip the `-b` flag and build by calling `make` directly in build/debug or build/release.

#### Dependecies
Sushi carries most dependencies as submodules and will build and link with them automatically. A few dependencies are not included however and must be provided or installed system-wide. See the list below:

  * libsndfile

  * alsa-lib

  * Jack2

  * Vst 2.4 SDK - Needs to be provided externally as it is not available from Steinberg anymore.

  * Raspa - Only required if building for an Elk Powered board and is included in the Elk cross compiling SDK.

  * gRPC - Can either be installed with packages libgrpc++-dev and protobuf-compiler-grpc from Ubuntu 20.04 LTS, or installed from source and installed system wide. See [https://github.com/grpc/grpc/blob/master/src/cpp/README.md] for instructions. Current gRPC version in native sushi is 1.16.1 with protobuf version 3.6.1. For Elk OS the grpc version is 1.24.1 with protobuf version 3.11. Other versions can't be guaranteed to work.

  * For LV2:

      * For Sushi:

          * liblilv-dev - at least version 0.24.4. Lilv is an official wrapper for LV2.
          * lilv-utils - at least version 0.24.5.
          * lv2-dev - at least version 1.18.2. The main LV2 library.

        The official Ubuntu repositories do not have these latest versions at the time of writing. The best source for them is instead the [KX Studio repositories, which you need to enable manually](https://kx.studio/Repositories).

      * For LV2 unit tests:

          * lv2-examples - at least version 1.18.2.
          * mda-lv2 - at least version 1.2.4 of [drobilla's port](http://drobilla.net/software/mda-lv2/) - not that from Mod Devices or others.

As an alternative to install most of dependencies through your Linux's system package manager, you can use vcpkg for Linux as well (check the following macOS section for instructions).

### Building for macOS
SUSHI includes vcpkg to manage third-party dependencies under macOS.

Building should be as simple as:

```
$ ./third-party/vcpkg/bootstrap-vcpkg.sh
$ mkdir build && cd build 
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../third-party/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```

This might take some time for the first build since all the vcpkg dependencies will have to be built first.

### Useful CMake build options
Various options can be passed to CMake directly, or through the generate script using the `--cmake-args` flag. Both general CMake options and Sushi-specific options that control which features Sushi is built with can be passed. Note that all options need to be prefixed with `-D` when passing them. All options are on be default as that is the most common use case.

Option                          | Value    | Default | Notes
--------------------------------|----------|---------|------------------------------------------------------------------------------------------------------
SUSHI_AUDIO_BUFFER_SIZE               | 8 - 512  | 64      | The buffer size used in the audio processing. Needs to be a power of 2 (8, 16, 32, 64, 128...).
SUSHI_WITH_XENOMAI                    | on / off | on      | Build Sushi with Xenomai RT-kernel support, only for ElkPowered hardware.
SUSHI_WITH_JACK                       | on / off | on      | Build Sushi with Jack Audio support, only for standard Linux distributions and macOS
SUSHI_WITH_PORTAUDIO                  | on / off | on      | Build Sushi with Portaudio support
SUSHI_WITH_ALSA_MIDI                  | on / off | on      | Build Sushi with Alsa sequencer support for MIDI (Linux only)
SUSHI_WITH_RT_MIDI                    | on / off | on      | Build Sushi with RtMidi support for MIDI. Cannot be selected if SUSHI_WITH_ALSA_MIDI is set.
SUSHI_WITH_LINK                       | on / off | on      | Build Sushi with Ableton Link support.
SUSHI_WITH_VST2                       | on / off | on      | Include support for loading Vst 2.x plugins in Sushi.
SUSHI_WITH_VST3                       | on / off | on      | Include support for loading Vst 3.x plugins in Sushi.
SUSHI_WITH_LV2                        | on / off | on      | Include support for loading LV2 plugins in Sushi. 
SUSHI_WITH_RPC_INTERFACE              | on / off | on      | Build gRPC external control interface, requires gRPC development files.
SUSHI_BUILD_TWINE                     | on / off | on      | Build and link with the included version of [TWINE](https://github.com/elk-audio/twine), otherwise tries to link with system wide if the option is disabled.
SUSHI_WITH_UNIT_TESTS                 | on / off | on      | Build and run unit tests together with building Sushi.
SUSHI_WITH_LV2_MDA_TESTS              | on / off | on      | Include LV2 unit tests which depends on the LV2 drobilla port of the mda plugins being installed. 
VST2_SDK_PATH                   | path     | empty   | Path to external Vst 2.4 SDK. Not included and required if WITH_VST2 is enabled.

_Note_:

before version 1.0, the Cmake options didn't have the `SUSHI_` prefix. The old names (e.g. `WITH_JACK`) will still be supported for some time but they shouldn't be used together with the new options format.

## License

Sushi is licensed under Affero General Public License (“AGPLv3”). See [LICENSE](LICENSE.md) document for the full details of the license. For contributing code to Sushi, see [CONTRIBUTING.md](CONTRIBUTING.md).

Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm, Sweden.

