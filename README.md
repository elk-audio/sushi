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

With JACK, Sushi creates 8 virtual input and output ports that you can connect to other programs or system outputs.

## Sushi macOS (experimental)
Since version 1.0, Sushi can be built natively for macOS as a native binary with all the dependencies statically linked to it.

There is a new Portaudio frontend (selectable with the `-a | --portaudio` command-line option) to interface directly with Portaudio. As an alternative, Jack can be used if it is available on the system.

With Portaudio, you can select other devices than the default with the `--audio-input-device` and `--audio-output-device` options. To find out the right number there, you can launch Sushi with the `--dump-portaudio-devs` to get a list in JSON format printed to stdout.

MIDI support is provided through RtMidi and can access directly CoreMidi devices.

LV2 support is currently not available for macOS.

## Example Sushi configuration files in repository
Under `misc/config_files` in this repository, we have a large variety of example Sushi configuration files.

They all use the mda-vst3 plugins which are built when building Sushi. If you are running one of the prebuilt packages (available on the releases sections on Github), you have everything inside the `sushi` folder there. For example, on macOS you should be able to get a simple working synthesizer with:
```
$ ./sushi -a -c config_files/play_vst3.json
```
(on Linux with JACK, replace `-a` with -`j`).

Otherwise, if you are building from source, the plugins used by the examples can be found under:

`build/debug/VST3/Debug`, or `build/release/VST3/Release` respectively, for debug and release builds.

To run Sushi with an example configuration, you simply invoke it while pointing to one of the above paths.

On Ubuntu that could be:
```
$ ./sushi -j -c ../../misc/config_files/play_vst3.json --base-plugin-path VST3/Debug
```

Or, from a macOS terminal:
```
$ ./sushi.app/Contents/MacOS/sushi -a -c ../../misc/config_files/play_vst3.json --base-plugin-path VST3/Release
```

## Extra documentation
Configuration files are used for global host configs, track and plugins configuration, MIDI routing and mapping, events sequencing.

More in-depth documentation is available at the [Elk Audio OS official docs page](https://elk-audio.github.io/elk-docs/html/documents/sushi_overview.html).

## Building
Sushi builds are supported for native Linux systems, Yocto/OE cross-compiling toolchains targeting Elk Audio OS systems, and macOS.

Make sure that Sushi is cloned with the `--recursive` flag to fetch all required submodules for building. Alternatively run `git submodule update --init --recursive` after cloning.

Sushi requires a compiler with support for C++17 features. The recommended compilers are GCC version 10 or higher, and clang version 13 or higher.

### Native Linux dependencies
Sushi handles most dependencies with vcpkg (or as submodules) and will build and link with them automatically. A few dependencies are not included however and must be provided or installed system-wide. See the list below (debian packages names):

  * libasound2-dev

  * libjack-jackd2-dev

  * For VST 2:
      * Vst 2.4 SDK - Needs to be provided externally as it is not available from Steinberg anymore.

  * For LV2:

      * liblilv-dev - at least version 0.24.4. Lilv is an official wrapper for LV2.
      * lilv-utils - at least version 0.24.5.
      * lv2-dev - at least version 1.18.2. The main LV2 library.

        The official Ubuntu repositories do not have these latest versions at the time of writing. The best source for them is instead the [KX Studio repositories, which you need to enable manually](https://kx.studio/Repositories).

      * For LV2 unit tests:

          * lv2-examples - at least version 1.18.2.
          * mda-lv2 - at least version 1.2.4 of [drobilla's port](http://drobilla.net/software/mda-lv2/) - not that from Mod Devices or others.

### Building with vcpkg (native Linux & macOS)
Instructions:

```
$ ./third-party/vcpkg/bootstrap-vcpkg.sh
$ mkdir build && cd build 
$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../third-party/vcpkg/scripts/buildsystems/vcpkg.cmake ..
```

This might take some time for the first build since all the vcpkg dependencies will have to be built first.

### Building with Yocto for Elk Audio OS
Sushi can be built either with the provided [Elk Audio OS SDK](https://github.com/elk-audio/elkpi-sdk), or as part of a [full Elk Audio OS image build with bitbake](https://github.com/elk-audio/elkpi-yocto-layers).

Follow the instructions in those repositories to set up a cross-compiling SDK and build Sushi for a given target.

Note: Sushi version 1.0 is not currently supported with Elk Audio OS images up to 0.12.0. The most recent supported Yocto version is tagged as `1.0-rc1_yocto_build` in the repository.

### Useful CMake build options
Option                                | Value    | Notes
--------------------------------------|----------|------------------------------------------------------------------------------------------------------
SUSHI_AUDIO_BUFFER_SIZE               | 8 - 512  | The buffer size used in the audio processing. Needs to be a power of 2 (8, 16, 32, 64, 128...).
SUSHI_WITH_XENOMAI                    | on / off | Build Sushi with Xenomai RT-kernel support, only for ElkPowered hardware.
SUSHI_WITH_JACK                       | on / off | Build Sushi with Jack Audio support, only for standard Linux distributions and macOS
SUSHI_WITH_PORTAUDIO                  | on / off | Build Sushi with Portaudio support
SUSHI_WITH_ALSA_MIDI                  | on / off | Build Sushi with Alsa sequencer support for MIDI (Linux only)
SUSHI_WITH_RT_MIDI                    | on / off | Build Sushi with RtMidi support for MIDI. Cannot be selected if SUSHI_WITH_ALSA_MIDI is set.
SUSHI_WITH_LINK                       | on / off | Build Sushi with Ableton Link support.
SUSHI_WITH_VST2                       | on / off | Include support for loading Vst 2.x plugins in Sushi.
SUSHI_WITH_VST3                       | on / off | Include support for loading Vst 3.x plugins in Sushi.
SUSHI_WITH_LV2                        | on / off | Include support for loading LV2 plugins in Sushi. 
SUSHI_WITH_RPC_INTERFACE              | on / off | Build gRPC external control interface, requires gRPC development files.
SUSHI_BUILD_TWINE                     | on / off | Build and link with the included version of [TWINE](https://github.com/elk-audio/twine), otherwise tries to link with system wide if the option is disabled.
SUSHI_TWINE_STATIC                    | on / off | Link statically against TWINE (not recommended, useful only in a few cases)
SUSHI_WITH_UNIT_TESTS                 | on / off | Build and run unit tests together with building Sushi.
SUSHI_WITH_LV2_MDA_TESTS              | on / off | Include LV2 unit tests which depends on the LV2 drobilla port of the mda plugins being installed. 
SUSHI_VST2_SDK_PATH                   | path     | Path to external Vst 2.4 SDK. Not included and required if WITH_VST2 is enabled.

The default values for the options are platform-specific (native Linux, Yocto/OE, macOS).

_Note_:

before version 1.0, the Cmake options didn't have the `SUSHI_` prefix. The old names (e.g. `WITH_JACK`) are not supported anymore and should be changed to the new format.

## Running Unit tests separately
Some Sushi's unit tests depend on test data, which is found through the environment variable `SUSHI_TEST_DATA_DIR`.
You will need to define this if you want to run the unit test explicitly, e.g. while debugging:

`$ export SUSHI_TEST_DATA_DIR=/path/to/sushi/repo/test/data`

## License
Sushi is licensed under Affero General Public License (“AGPLv3”). See [LICENSE](LICENSE.md) document for the full details of the license. For contributing code to Sushi, see [CONTRIBUTING.md](CONTRIBUTING.md).

Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm, Sweden.

