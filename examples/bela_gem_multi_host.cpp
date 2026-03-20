/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Minimal proof-of-concept Sushi Reactive host for Bela Gem Multi.
 *
 * The Bela Gem Multi exposes 10 analogue inputs and 10 analogue outputs.
 * This file shows the minimum scaffolding required to embed Sushi into a
 * Bela render loop using the Reactive frontend with a 10-in / 10-out
 * audio configuration.
 *
 * Build requirements
 * ------------------
 *   - Link against libsushi (built with the Reactive frontend enabled).
 *   - Include the Bela headers (Bela.h) from the Bela SDK.
 *   - Include twine headers (twine/twine.h) — a dependency of libsushi, available
 *     on all Xenomai-capable Bela builds.
 *   - SUSHI_AUDIO_BUFFER_SIZE must match the Bela block size selected at run-time
 *     (e.g. pass -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=64 to CMake when the Bela block
 *     is set to 64 frames).
 *
 * Channel mapping
 * ---------------
 *   Bela Gem Multi hardware channel order matches Sushi's internal layout
 *   (non-interleaved, channel-major).  No re-ordering is needed; channels
 *   are passed straight through.
 *
 * Timestamp strategy
 * ------------------
 *   Bela's render() runs on a Xenomai Cobalt thread.  twine::current_rt_time()
 *   returns the Cobalt monotonic clock, which is the same epoch used by Sushi's
 *   Ableton Link implementation.  Passing it directly as the process timestamp:
 *     - eliminates float-precision drift (no sample-counter arithmetic)
 *     - enables automatic xrun detection inside ReactiveFrontend::process_audio()
 *     - makes Ableton Link sync clock-accurate
 *   calculate_timestamp_from_start() is intentionally NOT used here.
 *
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

// ---- Bela SDK ---------------------------------------------------------------
// When compiling outside a Bela project, stub these out or replace with the
// real Bela.h from the Bela SDK package.
#include <Bela.h>

// ---- Twine (Sushi RT-threading library, available on Xenomai Bela) ----------
#include <twine/twine.h>

// ---- Sushi reactive API -----------------------------------------------------
#include <sushi/sushi.h>
#include <sushi/reactive_factory.h>
#include <sushi/rt_controller.h>
#include <sushi/sample_buffer.h>
#include <sushi/constants.h>       // MAX_ENGINE_CV_IO_PORTS

// ---- Standard library -------------------------------------------------------
#include <cassert>
#include <cstring>
#include <memory>

// =============================================================================
// Constants
// =============================================================================

static constexpr int BELA_GEM_MULTI_INPUTS  = 10;
static constexpr int BELA_GEM_MULTI_OUTPUTS = 10;

// Bela Gem Multi uses double-buffering: output latency is two block periods.
// This is passed to Sushi so the engine can schedule plugin automation and
// MIDI output against the correct wall-clock position.
static int bela_output_latency_us(float sample_rate, uint32_t block_size)
{
    return static_cast<int>(2u * block_size * 1'000'000u / static_cast<uint32_t>(sample_rate));
}

// =============================================================================
// Module-level state (lives for the duration of the Bela session)
// =============================================================================

struct BelaGemMultiSushiHost
{
    std::unique_ptr<sushi::Sushi>        sushi;
    std::unique_ptr<sushi::RtController> rt_controller;

    // Sushi works on fixed-size, non-interleaved ChunkSampleBuffers.
    // Bela Gem Multi delivers non-interleaved audio, so we can wrap
    // context->audioIn zero-copy and own only the output buffer.
    sushi::ChunkSampleBuffer buffer_out {BELA_GEM_MULTI_OUTPUTS};

    float    sample_rate   {44100.0f};
    uint32_t bela_block_size {0};
};

static BelaGemMultiSushiHost g_host;

// =============================================================================
// Bela callbacks
// =============================================================================

/**
 * @brief Called once before the audio loop begins.
 *        Instantiates and starts a Reactive Sushi instance configured for
 *        10 audio inputs and 10 audio outputs.
 */
bool setup(BelaContext* context, void* /*userData*/)
{
    g_host.sample_rate     = static_cast<float>(context->audioSampleRate);
    g_host.bela_block_size = context->audioFrames;

    // Verify the block size matches the compile-time Sushi chunk size.
    // If they differ, the host must split or accumulate host buffers before
    // calling process_audio().
    assert(g_host.bela_block_size == static_cast<uint32_t>(sushi::AUDIO_CHUNK_SIZE) &&
           "Bela block size must match SUSHI_CUSTOM_AUDIO_CHUNK_SIZE at build time");

    // ------------------------------------------------------------------
    // Sushi options
    // ------------------------------------------------------------------
    sushi::SushiOptions options;

    // Use the Reactive frontend (no threads, driven by render()).
    options.frontend_type = sushi::FrontendType::REACTIVE;

    // 10-in / 10-out — the whole point of this host.
    options.reactive_audio_inputs  = BELA_GEM_MULTI_INPUTS;
    options.reactive_audio_outputs = BELA_GEM_MULTI_OUTPUTS;

    // Tell the engine about the hardware output latency so that scheduled
    // parameter automation and MIDI arrive at the physical output on time.
    options.reactive_output_latency_us =
        bela_output_latency_us(g_host.sample_rate, g_host.bela_block_size);

    // Provide a JSON config that sets up tracks with matching channel counts.
    // Swap ConfigurationSource::NONE for ConfigurationSource::FILE and set
    // config_filename to load a real patch.
    options.config_source = sushi::ConfigurationSource::NONE;

    // Disable gRPC and OSC for a minimal embedded deployment.
    options.use_grpc = false;
    options.use_osc  = false;

    options.log_level = "warning";

    // ------------------------------------------------------------------
    // Instantiate via ReactiveFactory (single-use object)
    // ------------------------------------------------------------------
    sushi::ReactiveFactory factory;
    auto [sushi_instance, status] = factory.new_instance(options);

    if (status != sushi::Status::OK)
    {
        rt_printf("Sushi init failed: %s\n", sushi::to_string(status).c_str());
        return false;
    }

    g_host.sushi         = std::move(sushi_instance);
    g_host.rt_controller = factory.rt_controller();

    // ------------------------------------------------------------------
    // Optional: register MIDI output callback
    // ------------------------------------------------------------------
    g_host.rt_controller->set_midi_callback(
        [](int /*output*/, sushi::MidiDataByte data, sushi::Time /*timestamp*/)
        {
            // Forward MIDI output from Sushi back to the Bela MIDI subsystem.
            // Replace with actual Bela MIDI send calls as needed.
            (void)data;
        }
    );

    // ------------------------------------------------------------------
    // Start Sushi and set the sample rate
    // ------------------------------------------------------------------
    auto start_status = g_host.sushi->start();
    if (start_status != sushi::Status::OK)
    {
        rt_printf("Sushi start() failed: %s\n", sushi::to_string(start_status).c_str());
        return false;
    }

    g_host.sushi->set_sample_rate(g_host.sample_rate);

    rt_printf("Sushi reactive host ready: %d-in / %d-out @ %.0f Hz, block=%u, latency=%d us\n",
              BELA_GEM_MULTI_INPUTS, BELA_GEM_MULTI_OUTPUTS,
              g_host.sample_rate, g_host.bela_block_size,
              options.reactive_output_latency_us);

    return true;
}

/**
 * @brief Real-time audio callback — called once per block by the Bela audio engine.
 *
 * Bela Gem Multi delivers audio as non-interleaved float arrays
 * (context->audioIn / context->audioOut) with layout float[channel][frame].
 * Sushi's ChunkSampleBuffer uses the same layout, so audioIn is wrapped
 * zero-copy via create_from_raw_pointer().
 *
 * Timestamp strategy
 * ------------------
 * twine::current_rt_time() is called at the top of render() to capture the
 * hardware Cobalt clock at the precise moment the DMA interrupt fired.
 * This timestamp is passed directly to process_audio() and also handed to
 * increment_samples_since_start() so that RealTimeController::_clock_anchor
 * is updated every block.  Consequences:
 *
 *   - ReactiveFrontend::_handle_resume() compares it against the previous
 *     callback's timestamp and fires notify_interrupted_audio() automatically
 *     on any dropout, without the host needing to detect or report xruns.
 *   - calculate_timestamp_from_start() (for any code paths that use it) will
 *     anchor to the real Cobalt clock rather than a pure sample counter,
 *     preventing float-precision drift and aligning with Ableton Link.
 */
void render(BelaContext* context, void* /*userData*/)
{
    // Capture hardware timestamp at the start of this block.
    // render() runs on a Xenomai Cobalt thread, making current_rt_time() safe.
    sushi::Time timestamp = twine::current_rt_time();

    // ------------------------------------------------------------------
    // Wrap Bela's non-interleaved input array directly (zero-copy read).
    // audioIn layout: float[channel][frame], stride = context->audioFrames
    // Sushi SampleBuffer layout: float[channel][AUDIO_CHUNK_SIZE]
    // Both match when bela_block_size == AUDIO_CHUNK_SIZE.
    // ------------------------------------------------------------------
    auto wrapped_in = sushi::ChunkSampleBuffer::create_from_raw_pointer(
        const_cast<float*>(context->audioIn),
        0,
        BELA_GEM_MULTI_INPUTS);

    // ------------------------------------------------------------------
    // CV inputs: Bela's analogIn provides up to 8 channels at half the
    // audio sample rate.  Read the last sample of each analog frame as
    // the representative value for this audio block, normalise from
    // Bela's [0, 1] range directly (Bela analog range is already [0,1]).
    // Only route channels that Sushi was configured to accept.
    // ------------------------------------------------------------------
    {
        int cv_channels = std::min(context->analogInChannels,
                                   sushi::MAX_ENGINE_CV_IO_PORTS);
        int last_analog_frame = static_cast<int>(context->analogFrames) - 1;
        for (int ch = 0; ch < cv_channels; ++ch)
        {
            float value = analogRead(context, last_analog_frame, ch);
            g_host.rt_controller->set_cv_input(ch, value);
        }
    }

    // ------------------------------------------------------------------
    // Gate inputs: Bela's digital pins (0-15) mapped to Sushi gates.
    // ------------------------------------------------------------------
    {
        int gate_count = std::min(context->digitalChannels, 16);
        for (int g = 0; g < gate_count; ++g)
        {
            bool high = digitalRead(context, 0, g) != 0;
            g_host.rt_controller->set_gate_input(g, high);
        }
    }

    // ------------------------------------------------------------------
    // Drive Sushi — xrun detection and pause handling happen inside
    // ReactiveFrontend::process_audio() via _handle_resume/_handle_pause.
    // ------------------------------------------------------------------
    g_host.rt_controller->process_audio(wrapped_in, g_host.buffer_out, timestamp);

    // ------------------------------------------------------------------
    // CV outputs: write Sushi's CV results back to Bela's analog outputs.
    // ------------------------------------------------------------------
    {
        int cv_channels = std::min(context->analogOutChannels,
                                   sushi::MAX_ENGINE_CV_IO_PORTS);
        for (int ch = 0; ch < cv_channels; ++ch)
        {
            float value = g_host.rt_controller->cv_output(ch);
            for (uint32_t f = 0; f < context->analogFrames; ++f)
            {
                analogWrite(context, f, ch, value);
            }
        }
    }

    // ------------------------------------------------------------------
    // Gate outputs: drive Bela's digital pins from Sushi's gate results.
    // ------------------------------------------------------------------
    {
        int gate_count = std::min(context->digitalChannels, 16);
        for (int g = 0; g < gate_count; ++g)
        {
            digitalWrite(context, 0, g,
                         g_host.rt_controller->gate_output(g) ? HIGH : LOW);
        }
    }

    // ------------------------------------------------------------------
    // Copy Sushi output into Bela's audioOut array.
    // audioOut layout: float[channel][frame]
    // ------------------------------------------------------------------
    for (int ch = 0; ch < BELA_GEM_MULTI_OUTPUTS; ++ch)
    {
        std::memcpy(context->audioOut + ch * context->audioFrames,
                    g_host.buffer_out.channel(ch),
                    sizeof(float) * context->audioFrames);
    }

    // ------------------------------------------------------------------
    // Advance the sample counter and update the real-clock anchor.
    // Passing the hardware timestamp here keeps _clock_anchor current so
    // that calculate_timestamp_from_start() returns clock-accurate values
    // if anything inside Sushi calls it between now and the next block.
    // ------------------------------------------------------------------
    g_host.rt_controller->increment_samples_since_start(
        static_cast<int64_t>(context->audioFrames), timestamp);
}

/**
 * @brief Called once after the audio loop ends.
 */
void cleanup(BelaContext* /*context*/, void* /*userData*/)
{
    if (g_host.sushi)
    {
        g_host.rt_controller.reset();
        g_host.sushi->stop();
        g_host.sushi.reset();
    }
}
