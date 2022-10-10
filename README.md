# SUSHI
Headless plugin host for ELK Audio OS.

## Usage

See `sushi -h` for a complete list of options.
Common use cases are:

Test in offline mode with I/O from audio file:

    $ sushi -o -i input_file.wav -c config_file.json

Use JACK for realtime audio:

    $ sushi -j -c config_file.json

Use Portaudio for realtime audio, with the default devices:
    $ sushi -a -c config_file.json

With JACK, sushi creates 8 virtual input and output ports that you can connect to other programs or system outputs.

## SUSHI macOS (experimental)
Since version 1.0, SUSHI can be built natively for macOS as a native binary with all the dependencies statically linked to it.

There is a new Portaudio frontend (selectable with the `-a | --portaudio` command-line option) to interface directly with Portaudio. As an alternative, Jack can be used if it is available on the system.

With Portaudio, you can select other devices than the default with the `--audio-input-device` and `--audio-output-device` options. To find out the right number there, you can launch SUSHI with the `--dump-portaudio-devs` to get a list in JSON format printed to stdout.

MIDI support is provided through RtMidi and can access directly CoreMidi devices.

LV2 support is currently not available for macOS.

## Configuration file examples

See directory `example_configs` for the JSON-schema definition and some example configurations.
Configuration files are used for global host configs, track and plugins configuration, MIDI routing and mapping, events sequencing.

## Building Sushi
Sushi uses CMake as its build system. Just run the canonical:
```
  $ mkdir build && cd build
  $ cmake .. -DCMAKE_BUILD_TYPE=[Release|Debug]
```

to prepare the build environment.

Make sure that Sushi is cloned with the `--recursive` flag to fetch all required submodules for building. Alternatively run `git submodule update --init --recursive` after cloning.

Sushi requires a compiler with support for C++17 features. The recommended compiler is GCC version 10 or higher.


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

As an alternative to installing most dependencies through your Linux system’s package manager, you can use vcpkg for Linux as well (check the following macOS section for instructions).

### Building for macOS
SUSHI includes vcpkg to manage third-party dependencies under macOS.

Instructions:

```
$ ./third-party/vcpkg/bootstrap-vcpkg.sh
$ mkdir build && cd build 
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../third-party/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```

This might take some time for the first build since all the vcpkg dependencies will have to be built first.

### Useful CMake build options
Various options can be passed to CMak. Both general CMake options and Sushi-specific options that control which features Sushi is built with can be passed. Note that all options need to be prefixed with `-D` when passing them. All options are on be default as that is the most common use case.

Option                                | Value    | Default* | Notes
--------------------------------------|----------|----------|------------------------------------------------------------------------------------------------------
SUSHI_AUDIO_BUFFER_SIZE               | 8 - 512  | 64       | The buffer size used in the audio processing. Needs to be a power of 2 (8, 16, 32, 64, 128...).
SUSHI_WITH_XENOMAI                    | on / off | on (off) | Build Sushi with Xenomai RT-kernel support, only for ElkPowered hardware.
SUSHI_WITH_JACK                       | on / off | off      | Build Sushi with Jack Audio support, only for standard Linux distributions and macOS
SUSHI_WITH_PORTAUDIO                  | on / off | on       | Build Sushi with Portaudio support
SUSHI_WITH_ALSA_MIDI                  | on / off | off (on) | Build Sushi with Alsa sequencer support for MIDI (Linux only)
SUSHI_WITH_RT_MIDI                    | on / off | on (off) | Build Sushi with RtMidi support for MIDI. Cannot be selected if SUSHI_WITH_ALSA_MIDI is set.
SUSHI_WITH_LINK                       | on / off | on       | Build Sushi with Ableton Link support.
SUSHI_WITH_VST2                       | on / off | on (off) | Include support for loading Vst 2.x plugins in Sushi.
SUSHI_WITH_VST3                       | on / off | on       | Include support for loading Vst 3.x plugins in Sushi.
SUSHI_WITH_LV2                        | on / off | on (off) | Include support for loading LV2 plugins in Sushi. 
SUSHI_WITH_RPC_INTERFACE              | on / off | on       | Build gRPC external control interface, requires gRPC development files.
SUSHI_BUILD_TWINE                     | on / off | off (on) | Build and link with the included version of [TWINE](https://github.com/elk-audio/twine), otherwise tries to link with system wide if the option is disabled.
SUSHI_WITH_UNIT_TESTS                 | on / off | on       | Build and run unit tests together with building Sushi.
SUSHI_WITH_LV2_MDA_TESTS              | on / off | on (off) | Include LV2 unit tests which depends on the LV2 drobilla port of the mda plugins being installed. 
SUSHI_VST2_SDK_PATH                   | path     | empty    | Path to external Vst 2.4 SDK. Not included and required if WITH_VST2 is enabled.

_*_:
the defaults vary depending on the target platform, in the table they are for Linux (macOS).

_Note_:

before version 1.0, the Cmake options didn't have the `SUSHI_` prefix. The old names (e.g. `WITH_JACK`) are not supported anymore and should be changed to the new format.

## License

Sushi is licensed under Affero General Public License (“AGPLv3”). See [LICENSE](LICENSE.md) document for the full details of the license. For contributing code to Sushi, see [CONTRIBUTING.md](CONTRIBUTING.md).

Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm, Sweden.

