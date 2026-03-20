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
 *   - SUSHI_AUDIO_BUFFER_SIZE must match the Bela block size selected at
 *     run-time (e.g. pass -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=64 to CMake when
 *     the Bela block is set to 64 frames).
 *
 * Channel mapping
 * ---------------
 *   Bela Gem Multi hardware channel order matches Sushi's internal layout
 *   (non-interleaved, channel-major).  No re-ordering is needed; channels
 *   are passed straight through.
 *
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

// ---- Bela SDK ---------------------------------------------------------------
// When compiling outside a Bela project, stub these out or replace with the
// real Bela.h from the Bela SDK package.
#include <Bela.h>

// ---- Sushi reactive API -----------------------------------------------------
#include <sushi/sushi.h>
#include <sushi/reactive_factory.h>
#include <sushi/rt_controller.h>
#include <sushi/sample_buffer.h>

// ---- Standard library -------------------------------------------------------
#include <cassert>
#include <cstring>
#include <memory>

// =============================================================================
// Constants
// =============================================================================

static constexpr int BELA_GEM_MULTI_INPUTS  = 10;
static constexpr int BELA_GEM_MULTI_OUTPUTS = 10;

// =============================================================================
// Module-level state (lives for the duration of the Bela session)
// =============================================================================

struct BelaGemMultiSushiHost
{
    std::unique_ptr<sushi::Sushi>        sushi;
    std::unique_ptr<sushi::RtController> rt_controller;

    // Sushi works on fixed-size, non-interleaved ChunkSampleBuffers.
    // Bela delivers interleaved samples through context->audioIn / audioOut.
    sushi::ChunkSampleBuffer buffer_in  {BELA_GEM_MULTI_INPUTS};
    sushi::ChunkSampleBuffer buffer_out {BELA_GEM_MULTI_OUTPUTS};

    float    sample_rate {44100.0f};
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
    g_host.sample_rate    = static_cast<float>(context->audioSampleRate);
    g_host.bela_block_size = context->audioFrames;

    // Verify the block size matches the compile-time Sushi chunk size.
    // If they differ you must accumulate / split host buffers externally.
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

    rt_printf("Sushi reactive host ready: %d-in / %d-out @ %.0f Hz, block=%u\n",
              BELA_GEM_MULTI_INPUTS, BELA_GEM_MULTI_OUTPUTS,
              g_host.sample_rate, g_host.bela_block_size);

    return true;
}

/**
 * @brief Real-time audio callback — called once per block by the Bela audio engine.
 *
 * The Bela Gem Multi delivers audio as non-interleaved float arrays
 * (context->audioIn / audioOut), with shape [channel][frame].
 * Sushi's ChunkSampleBuffer uses the same non-interleaved layout, so we can
 * wrap the Bela pointers directly via create_from_raw_pointer() for zero-copy
 * input.  Output is written into an owned buffer and then copied back.
 */
void render(BelaContext* context, void* /*userData*/)
{
    // ------------------------------------------------------------------
    // Wrap Bela's non-interleaved input array directly (zero-copy read).
    // audioIn layout: float[channel][frame], stride = context->audioFrames
    // Sushi SampleBuffer layout: float[channel][AUDIO_CHUNK_SIZE]
    // Both are the same when bela_block_size == AUDIO_CHUNK_SIZE.
    // ------------------------------------------------------------------
    auto wrapped_in = sushi::ChunkSampleBuffer::create_from_raw_pointer(
        const_cast<float*>(context->audioIn),
        0,
        BELA_GEM_MULTI_INPUTS);

    // ------------------------------------------------------------------
    // Calculate timestamp
    // ------------------------------------------------------------------
    sushi::Time timestamp = g_host.rt_controller->calculate_timestamp_from_start(
        g_host.sample_rate);

    // ------------------------------------------------------------------
    // Drive Sushi
    // ------------------------------------------------------------------
    g_host.rt_controller->process_audio(wrapped_in, g_host.buffer_out, timestamp);

    // ------------------------------------------------------------------
    // Copy Sushi output into Bela's audioOut array
    // audioOut layout: float[channel][frame]
    // ------------------------------------------------------------------
    for (int ch = 0; ch < BELA_GEM_MULTI_OUTPUTS; ++ch)
    {
        std::memcpy(context->audioOut + ch * context->audioFrames,
                    g_host.buffer_out.channel(ch),
                    sizeof(float) * context->audioFrames);
    }

    // ------------------------------------------------------------------
    // Advance the internal timestamp counter
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
