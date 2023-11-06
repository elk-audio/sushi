# Sushi as a Library
Using Sushi as a statically linked library is a newly developed feature.

We have considered two primary use-cases for Sushi as a library:

**Reactive:** 

Sushi running inside a plugin / other audio host, which provides the audio and MIDI frontends, passing on audio and MIDI to Sushi.

**Active:** 

Allowing the creation of standalone audio-hosts, which embed Sushi, but expand on the functionality of the terminal application we provide in `/apps`.

For the Reactive use-case, we have developed new frontends for audio and MIDI in Sushi, designed to be Reactive, as well as a new "Real-Time" Controller interface using which the necessary Sushi control is available in the audio callback. These are still in their first incarnation, and while they are definitely usable in production, they are currently missing features. That’s also why these files may still contain “TODO’s” as placeholders - to be removed as the features they refer to are added.

For the Active use-case, there are no such limitations - the full Sushi functionality is available. 

## The main limitations for the Reactive use-case are:
* Sushi can only work with stereo audio I/O.
* Sushi's audio buffer size is set at compile time, usign the CMake argument SUSHI_AUDIO_BUFFER_SIZE. But an audio host may have a dynamic buffer-size setting. If the buffer size doesn't match, the host needs to handle that, ensuring Sushi is only ever given audio buffers of the size defined at build-time.
* MIDI I/O to Sushi from a containing host application is not currently real-time, but asynchronous, meaning MIDI and audio synchronisation is not sample-accurate.
* For any control commands to be processes by Sushi, it needs to be receiving audio buffers regularly. When no audio callbacks are received, Sushi will also not process and control commands (e.g. those received over gRPC and OSC).

## Using Sushi as a library
For using Sushi as a library, you only need to be aware of the header files contained in the include/sushi folder, where Sushi's API is exposed. The internals are hidden away.

Some of those headers are useful only when Sushi is Passive, others only when it's Active, and others for both cases.

## Common API for Reactive and Active

Sushi has several compile-time arguments and dependencies, which apply equally to the library and standalone use. These are documented in the main [README.md](../README.md) of this repository, and will not be repeated here.

### Instantiation

The main Sushi API is defined in sushi.h.

All options for instantiating Sushi are collected in the struct **SushiOptions**. Some of its fields are enum classes - which are then also defined in sushi.h.

This file also contains the main abstract base-class for Sushi, which however cannot be instantiated directly. Instead, one of the provided factories needs to be used. Simply create a factory, and invoke the below method, passing your populated SushiOptions struct:

```c++
std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) override;
```

It will return a std::pair, of either a valid Sushi instance, and Status::OK, or an empty std::unique_ptr, and the Status of why instantiation failed.

Note that the SushiOptions is passed by reference to new_instance, because its contents are *suggestions*, which may be overridden if needed - in which case your options struct instance will be updated to reflect the changes made.

### Controlling during run-time

#### C++ Control API

The main control interface is exposed in **control_interface.h**.

Note that none of those classes can be instantiated directly. They are created internally in Sushi, and their instances are accessed through the **Sushi::Controller()** method, which returns a pointer to a **SushiControl** instance, that in turn will hold instances for all the interfaces defined in control_interface.h:

```C++
control::SushiControl* controller();
```

It's important to note that none of these interfaces are real-time safe. That means they are not safely callable from within an audio-callback, but should be invoked from a separate thread altogether. Sushi then schedules their effect internally so that they do not interfere with the real-time audio callback.

The C++ control API in control_interface.h is extensive and will not be further documented here. It is written so that it should be intuitively self-explanatory.

#### Remote Control API

Additionally, Sushi can be controlled over gRPC and OSC, which are both unchanged in the "sushi as library" implementation - see the documentation for this in the main [README.md](../README.md) of this repository, and the [Elk Audio GitHub page with detaled documentation](https://elk-audio.github.io/elk-docs/html/index.html).

Once Sushi is instantiated, subsequent steps vary depending on the use-case.

## Reactive use-case

You will need the following fields:

```C++
sushi::SushiOptions _sushi_options;
std::unique_ptr<sushi::RtController> _rt_controller;
std::unique_ptr<sushi::Sushi> _sushi;
sushi::ChunkSampleBuffer _sushi_buffer_in;
sushi::ChunkSampleBuffer _sushi_buffer_out;
```

### Starting Reactive Sushi

For the Reactive use-case, fetch the **RtController** from a **ReactiveFactory** instance (Sushi factories are single-use). Assuming Sushi has started successfully, this will be a non-null std::unique_ptr, meaning you are taking over the ownership of the controller.

RtController is named as such because its methods are safe to invoke during a real-time audio callback.

```C++
sushi::ReactiveFactory factory;
auto [sushi, status] = factory.new_instance(_sushi_options);
_sushi = std::move (sushi);
_rt_controller = factory.rt_controller();
```

If you are going to use MIDI, pass a callback Lambda to it through your RtController instance, so that Sushi can output MIDI data to your host:

```C++
_rt_controller->set_midi_callback(
    [&] (int output, sushi::MidiDataByte data, sushi::Time /*timestamp*/)
    { 
    		// Handle MIDI data.
    }
);
```

If you're going to host Sushi in e.g. an audio plugin, which can pass a transport position to Sushi, you need to tell Sushi to not calculate its own transport position internally:

```C++
_rt_controller->set_position_source (sushi::TransportPositionSource::EXTERNAL);
```

Finally, start Sushi, and if successful, set the sample rate. If not, address any issue and retry:

```c++
_sushi_start_status = _sushi->start();
if (_sushi_start_status == sushi::Status::OK)
{
    _sushi->set_sample_rate(_sample_rate);
    return true; // Sushi is started.
}
else
{
    _sushi = nullptr;
    _rt_controller = nullptr;
    // The factory is single use. Address the error, and perhaps retry, with a new factory instance.
}
```

For example: If you are using gRPC, it is possible that Sushi failed to start because the configured gRPC port was taken. You can then increment the port number using the utility method provided, and retry.

```C++
bool incrementation_status = _sushi_options.increment_grpc_port_number();
if (!incrementation_status)
{
    // Starting Sushi failed: gRPC address is malformed.
    break;
}
```

### Invoking Sushi in the audio callback of your host

For each audio callback invocation, you need to do the following, always using the RtController  instance.

Pass MIDI to Sushi using receive_midi.

```C++
void receive_midi(int input, MidiDataByte data, Time timestamp);
```

If you're using an external transport position source: update the Sushi transport, using the following.

```C++
void set_tempo(float tempo);
void set_time_signature(control::TimeSignature time_signature);
void set_playing_mode(control::PlayingMode mode);
bool set_current_beats(double beat_time);
bool set_current_bar_beats(double bar_beat_count);
```

Fill your sushi::**ChunkSampleBuffer** input audio buffer, and call process_audio on the RtController.

```C++
void process_audio(ChunkSampleBuffer& in_buffer,
                   ChunkSampleBuffer& out_buffer,
                   Time timestamp)
```

This will populate the out_buffer. The sushi::**Time** `timestamp` may either be calculated using your hosts playhead data, or you need to use the RtController utility to calculate it, if you're using the Sushi internal transport.

```C++
sushi::Time calculate_timestamp_from_start(float sample_rate) const
```

In which case, after the process_audio call, you will then also need to increment_samples_since_start.

```C++
void increment_samples_since_start(uint64_t sample_count, Time timestamp)
```

There is more detail to the above process to handle e.g. MIDI, or non-matching buffer sizes. We will be publishing a concrete example, showing how to implement a concrete audio plugin which wraps Sushi, in due course.

### Stopping Reactive Sushi

To stop Sushi, you'll really only need to delete the controller, call stop() on your sushi instance, and then delete that too:

```C++
if (_sushi)
{
    _rt_controller.reset();
    _sushi->stop();
    _sushi.reset();
}
```

## Active use-case

For the active use-case the Sushi API footprint is a bit smaller. You will need the following fields.

```c++
SushiOptions options;
std::unique_ptr<FactoryInterface> factory;
std::unique_ptr<Sushi> sushi;
```

Then depending on whether you plan to use an `OFFLINE` frontend, or if you want Sushi to instantiate its own Audio frontend (the options are defined in sushi::FrontendType), you will instantiate either a **StandaloneFactory**.

```C++
factory = std::make_unique<StandaloneFactory>();
```

Or an **OfflineFactory**.

```C++
factory = std::make_unique<OfflineFactory>();
```

To populate SushiOptions, we provide a utility method for parsing options from the Sushi terminal arguments, in **terminal_utilities.h**.

```C++
ParseStatus parse_options(int argc, char* argv[], sushi::SushiOptions& options);
```

From there on, you instantiate Sushi.

```C++
auto [sushi, status] = factory->new_instance(options);
```

And on success, start it.

```C++
auto start_status = sushi->start();
```

From there on, you can wait in the creating thread, until the application needs to exit, at which point you just need to invoke stop().

```C++
sushi->stop();
```

## Examples

For examples of sushi as a library:

### Active use-case

See apps/main.cpp in this repository, which shows a terminal application use-case.

### Passive use-case

See the example JUCE-based project, residing in a dedicated repository, where we demonstrate how Sushi can be wrapped in a JUCE audio plugin.