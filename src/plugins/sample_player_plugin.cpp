#include <cassert>
#include <sndfile.h>
#include <iostream>
#include "sample_player_plugin.h"
#include "logging.h"


namespace sushi {
namespace sample_player_plugin {

MIND_GET_LOGGER;

SamplePlayerPlugin::SamplePlayerPlugin()
{}


SamplePlayerPlugin::~SamplePlayerPlugin()
{
    delete(_sample_buffer);
}


StompBoxStatus SamplePlayerPlugin::init(const StompBoxConfig &configuration)
{
    _configuration = configuration;
    _volume_parameter  = configuration.controller->register_float_parameter("volume", "Volume", 0.0f, new dBToLinPreProcessor(-120.0f, 36.0f));
    _attack_parameter  = configuration.controller->register_float_parameter("attack", "Attack", 0.0f, new FloatParameterPreProcessor(0.0f, 10.0f));
    _decay_parameter   = configuration.controller->register_float_parameter("decay", "Decay", 0.5f, new FloatParameterPreProcessor(0.0f, 10.0f));
    _sustain_parameter = configuration.controller->register_float_parameter("sustain", "Sustain", 1.0f, new FloatParameterPreProcessor(0.0f, 1.0f));
    _release_parameter = configuration.controller->register_float_parameter("release", "Release", 1.0f, new FloatParameterPreProcessor(0.0f, 10.0f));
    _sample_file_parameter = configuration.controller->register_string_parameter("sample_file", "Sample File", SAMPLE_FILE);

    for (auto& voice : _voices)
    {
        voice.set_samplerate(configuration.sample_rate);
    }
    auto status = load_sample_file(SAMPLE_FILE);
    if (status != StompBoxStatus::OK)
    {
        MIND_LOG_ERROR("Sample file not found");
    }
    return status;
}


void SamplePlayerPlugin::process_event(BaseEvent* event)
{
    switch (event->type())
    {
        case EventType::NOTE_ON:
        {
            bool voice_allocated = false;
            KeyboardEvent* key_event = static_cast<KeyboardEvent*>(event);
            for (auto& voice : _voices)
            {
                if (!voice.active())
                {
                    voice.note_on(key_event->note(), key_event->velocity(), event->sample_offset());
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
                        voice.note_on(key_event->note(), key_event->velocity(), event->sample_offset());
                        break;
                    }
                 }
            }
            break;
        }
        case EventType::NOTE_OFF:
        {
            KeyboardEvent* key_event = static_cast<KeyboardEvent*>(event);
            for (auto& voice : _voices)
            {
                if (voice.active() && voice.current_note() == key_event->note())
                {
                    voice.note_off(key_event->velocity(), event->sample_offset());
                    break;
                }
            }
            break;
        }
        default: break;
    }
}


void SamplePlayerPlugin::process(const SampleBuffer<AUDIO_CHUNK_SIZE>* /*in_buffer*/, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    float gain = _volume_parameter->value();
    float attack = _attack_parameter->value();
    float decay = _decay_parameter->value();
    float sustain = _sustain_parameter->value();
    float release = _release_parameter->value();

    _buffer.clear();
    out_buffer->clear();
    for (auto& voice : _voices)
    {
        voice.set_envelope(attack, decay, sustain, release);
        voice.render(_buffer);
    }
    out_buffer->add_with_gain(_buffer, gain);
}


StompBoxStatus SamplePlayerPlugin::load_sample_file(const std::string &file_name)
{
    SNDFILE*    sample_file;
    SF_INFO     soundfile_info = {};

    if (! (sample_file = sf_open(file_name.c_str(), SFM_READ, &soundfile_info)) )
    {
        return StompBoxStatus::ERROR;
    }
    assert(soundfile_info.channels == 1);

    // Read sample data
    _sample_buffer = new float[soundfile_info.frames];
    int samples = sf_readf_float(sample_file, _sample_buffer, soundfile_info.frames);
    assert(samples == soundfile_info.frames);

    _sample.set_sample(_sample_buffer, soundfile_info.frames);
    for (auto& voice : _voices)
    {
        voice.set_sample(&_sample);
    }
    return StompBoxStatus::OK;
}


}// namespace sample_player_plugin
}// namespace sushi