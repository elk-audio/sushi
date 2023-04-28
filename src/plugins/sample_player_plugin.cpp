/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Sampler plugin example to test event and sample handling
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>
#include <sndfile.h>

#include "sample_player_plugin.h"
#include "logging.h"

namespace sushi {
namespace sample_player_plugin {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sampleplayer");

constexpr auto PLUGIN_UID = "sushi.testing.sampleplayer";
constexpr auto DEFAULT_LABEL = "Sample player";
constexpr int SAMPLE_PROPERTY_ID = 0;

SamplePlayerPlugin::SamplePlayerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    [[maybe_unused]] bool str_pr_ok = register_property("sample_file", "Sample File", "");

    _volume_parameter  = register_float_parameter("volume", "Volume", "dB",
                                                  0.0f, -120.0f, 36.0f,
                                                  Direction::AUTOMATABLE,
                                                  new dBToLinPreProcessor(-120.0f, 36.0f));

    _attack_parameter  = register_float_parameter("attack", "Attack", "s",
                                                  0.0f, 0.0f, 10.0f,
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.0f, 10.0f));

    _decay_parameter   = register_float_parameter("decay", "Decay", "s",
                                                  0.0f, 0.0f, 10.0f,
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.0f, 10.0f));

    _sustain_parameter = register_float_parameter("sustain", "Sustain", "",
                                                  1.0f, 0.0f, 1.0f,
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.0f, 1.0f));

    _release_parameter = register_float_parameter("release", "Release", "s",
                                                  0.0f, 0.0f, 10.0f,
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.0f, 10.0f));

    assert(_volume_parameter && _attack_parameter && _decay_parameter && _sustain_parameter && _release_parameter && str_pr_ok);
    _max_input_channels = 0;
}

ProcessorReturnCode SamplePlayerPlugin::init(float sample_rate)
{
    _sample.set_sample(&_dummy_sample, 0);
    for (auto& voice : _voices)
    {
        voice.set_samplerate(sample_rate);
        voice.set_sample(&_sample);
    }

    return ProcessorReturnCode::OK;
}

void SamplePlayerPlugin::configure(float sample_rate)
{
    for (auto& voice : _voices)
    {
        voice.set_samplerate(sample_rate);
    }
    return;
}

void SamplePlayerPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    if (enabled == false)
    {
        _all_notes_off();
    }
}

void SamplePlayerPlugin::set_bypassed(bool bypassed)
{
    // Kill all voices in bypass so we dont have any hanging notes when turning back on
    if (bypassed)
    {
        _all_notes_off();
    }
    Processor::set_bypassed(bypassed);
}

SamplePlayerPlugin::~SamplePlayerPlugin()
{
    delete[] _sample_buffer;
}

void SamplePlayerPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::NOTE_ON:
        {
            if (_bypassed)
            {
                break;
            }
            bool voice_allocated = false;
            auto key_event = event.keyboard_event();
            SUSHI_LOG_DEBUG("Sample Player: note ON, num. {}, vel. {}",
                            key_event->note(), key_event->velocity());
            for (auto& voice : _voices)
            {
                if (!voice.active())
                {
                    voice.note_on(key_event->note(), key_event->velocity(), event.sample_offset());
                    voice_allocated = true;
                    break;
                }
            }
            // TODO - improve voice stealing algorithm
            if (!voice_allocated)
            {
                for (auto& voice : _voices)
                {
                    if (voice.stopping())
                    {
                        voice.note_on(key_event->note(), key_event->velocity(), event.sample_offset());
                        break;
                    }
                }
            }
            break;
        }
        case RtEventType::NOTE_OFF:
        {
            if (_bypassed)
            {
                break;
            }
            auto key_event = event.keyboard_event();
            SUSHI_LOG_DEBUG("Sample Player: note OFF, num. {}, vel. {}",
                            key_event->note(), key_event->velocity());
            for (auto& voice : _voices)
            {
                if (voice.active() && voice.current_note() == key_event->note())
                {
                    voice.note_off(key_event->velocity(), event.sample_offset());
                    break;
                }
            }
            break;
        }

        case RtEventType::NOTE_AFTERTOUCH:
        case RtEventType::PITCH_BEND:
        case RtEventType::AFTERTOUCH:
        case RtEventType::MODULATION:
        case RtEventType::WRAPPED_MIDI_EVENT:
            // Consume these events so they are not propagated
            break;

        case RtEventType::DATA_PROPERTY_CHANGE:
        {
            // Kill all voices before swapping out the sample
            for (auto& voice : _voices)
            {
                voice.note_off(1.0f, 0);
            }

            auto typed_event = event.data_parameter_change_event();
            auto new_sample = typed_event->value();
            float* old_sample = _sample_buffer;
            _sample_buffer = reinterpret_cast<float*>(new_sample.data);
            _sample.set_sample(_sample_buffer, new_sample.size / sizeof(float));

            // Delete the old sample data outside the rt thread
            BlobData data{0, reinterpret_cast<uint8_t*>(old_sample)};
            auto delete_event = RtEvent::make_delete_blob_event(data);
            output_event(delete_event);
            break;
        }

        default:
            InternalPlugin::process_event(event);
            break;
    }
}


void SamplePlayerPlugin::process_audio(const ChunkSampleBuffer& /* in_buffer */, ChunkSampleBuffer& out_buffer)
{
    float gain = _volume_parameter->processed_value();
    float attack = _attack_parameter->processed_value();
    float decay = _decay_parameter->processed_value();
    float sustain = _sustain_parameter->processed_value();
    float release = _release_parameter->processed_value();

    _buffer.clear();
    out_buffer.clear();
    for (auto& voice : _voices)
    {
        voice.set_envelope(attack, decay, sustain, release);
        voice.render(_buffer);
    }
    if (!_bypassed)
    {
        out_buffer.add_with_gain(_buffer, gain);
    }
}

ProcessorReturnCode SamplePlayerPlugin::set_property_value(ObjectId property_id, const std::string& value)
{
    if (property_id == SAMPLE_PROPERTY_ID)
    {
        auto sample_data = _load_sample_file(value);
        if (sample_data.size > 0)
        {
            send_data_to_realtime(sample_data, 0);
        }
    }
    return InternalPlugin::set_property_value(property_id, value);
}

std::string_view SamplePlayerPlugin::static_uid()
{
    return PLUGIN_UID;
}

void SamplePlayerPlugin::_all_notes_off()
{
    for (auto &voice : _voices)
    {
        voice.note_off(1.0f, 0);
    }
}

BlobData SamplePlayerPlugin::_load_sample_file(const std::string& file_name)
{
    SNDFILE* sample_file;
    SF_INFO  soundfile_info = {};
    if (! (sample_file = sf_open(file_name.c_str(), SFM_READ, &soundfile_info)))
    {
        SUSHI_LOG_ERROR("Failed to open sample file: {}", file_name);
        return {0, nullptr};
    }

    int samples = 0;
    float* sample_buffer = new float[soundfile_info.frames];
    if (soundfile_info.channels == 1)
    {
        samples = sf_readf_float(sample_file, sample_buffer, soundfile_info.frames);
    }
    else // Decode interleaved stereo
    {
        float buffer[2];
        for (int i = 0; i < soundfile_info.frames; ++i)
        {
            samples += sf_readf_float(sample_file, buffer, 1);
            sample_buffer[i] = buffer[0];
        }
    }

    sf_close(sample_file);

    if (samples <= 0)
    {
        delete[] sample_buffer;
        return {0, nullptr};
    }

    return BlobData{static_cast<int>(soundfile_info.frames * sizeof(float)), reinterpret_cast<uint8_t*>(sample_buffer)};

}

}// namespace sample_player_plugin
}// namespace sushi
