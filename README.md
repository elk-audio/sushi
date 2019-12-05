# SUSHI
## Sensus Universal Sound Host Interface

Headless plugin host for ELK Audio OS.

## Usage

See `sushi -h` for a complete list of options.
Common use cases are:

Test in offline mode with I/O from audio file:

    $ sushi -o -i input_file.wav -c config_file.json

Use JACK for realtime audio:

    $ sushi -j -c config_file.json

With JACK, sushi creates 8 virtual input and output ports that you can connect to other programs or system outputs.

## Configuration file examples

See directory `example_configs` for the JSON-schema definition and some example configurations.
Configuration files are used for global host configs, track and plugins configuration, MIDI routing and mapping, events sequencing.

## Building Sushi
Sushi uses CMake as its build system. A generate script is also provided for convenient setup. Simply running `./generate` with no arguments in the root of Sushi will setup a build folder containing a Release configuration and a Debug configuration. CMake arguments can be passed through the generate script using the `--cmake-args` flag. Those arguments will then be added to both configurations.

### Useful CMake build options
Various options can be passed to CMake directly, or through the generate script using the `--cmake-args` flag. Both general CMake options and Sushi-specific options that control which features Sushi is built with can be passed. Note that all options need to be prefixed with `-D` when passing them. All options are on be default as that is the most common use case.

Option                          | Value    | Default | Notes
--------------------------------|----------|---------|------------------------------------------------------------------------------------------------------
AUDIO_BUFFER_SIZE               | 8 - 512  | 64      | The buffer size used in the audio processing. Needs to be a power of 2 (8, 16, 32, 64, 128...).
WITH_XENOMAI					| on / off | on      | Build Sushi with Xenomai RT-kernel support, only for ElkPowered hardware.
WITH_JACK	                    | on / off | on      | Build Sushi with Jack Audio support, only for standard Linux distributions.
WITH_VST2                       | on / off | on      | Include support for loading Vst 2.x plugins in Sushi.
VST2_SDK_PATH                   | path     | empty   | Path to external Vst 2.4 SDK. Not included and required if WITH_VST2 is enabled.
WITH_VST3                       | on / off | on      | Include support for loading Vst 3.x plugins in Sushi.
WITH_RPC_INTERFACE              | on / off | on      | Build gRPC external control interface, requires gRPC development files.
WITH_TWINE                      | on / off | on      | Build and link with the included version of TWINE, tries to link with system wide TWINE if option is disabled.
WITH_UNIT_TESTS                 | on / off | on      | Build and run unit tests together with building Sushi.

### Building for native Linux
As all options are on by default, to build Sushi for a regular, non Elk Powered Linux distribution, the Xenomai options need to be turned off. In addition the Vst 2.x SDK needs to be provided.

    $ ./generate --cmake-args="-DWITH_XENOMAI=off -DVST2_SDK_PATH=/home/elk/sdks/vstsdk2.4" -b

If the Vst 2.4 SDK is not available, Sushi can still be built without Vst 2.x support using the command below

    $ ./generate --cmake-args="-DWITH_XENOMAI=off -DWITH_VST2=off" -b

It is also possible to skip the `-b` flag and build by calling `make` directly in build/debug or build/release

### Dependecies
Sushi carries most dependencies as submodules and will build and link them automatically. A couple of depencies are not included however and must be provided or installed system-wide. See the list below:

  * libsndfile
  * alsa-lib
  * Jack2
  * Vst 2.4 SDK - Needs to be provided externally as it is not available from Steinberg anymore.
  * Raspa - Only required if building for an Elk Powered board and is included in out cross compiling SDK
  * gRPC - Needs to be built from source and installed system wide. See [https://github.com/grpc/grpc/blob/master/src/cpp/README.md] for instructions. The current version of gRPC used by sushi is 1.10.1. Other versions can not be guaranteed to work.


## Run-time OSC control

Sushi listens on port 24024 by default for the following OSC commands:

Path                                   | Typespec | Arguments
---------------------------------------|----------|--------------------------------------
/parameter/plugin_id/parameter_name    |  f       | parameter value
/keyboard_event/track_name             |  sif     | event type ("note_on", "note_off", "program_change"), note index, norm. value
/engine/add_track                      |  si      | name, n. of channels
/engine/delete_track                   |  s       | name
/engine/add_processor                  |  sssss   | track, id, name, file path, type
/engine/delete_processor               |  ss      | track name, plugin id
/engine/set_tempo                      |  f       | tempo in beats per minute
/engine/set_time_signature             |  ii      | time signature numerator, time signature denominator
/engine/set_playing_mode               |  s       | "playing" or "stopped"
/engine/set_sync_mode                  |  s       | "internal", "ableton_link" or "midi"

2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm, Sweden.
