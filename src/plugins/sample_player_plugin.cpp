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
    _decay_parameter   = configuration.controller->register_float_parameter("decay", "Decay", 0.0f, new FloatParameterPreProcessor(0.0f, 10.0f));
    _sustain_parameter = configuration.controller->register_float_parameter("sustain", "Sustain", 1.0f, new FloatParameterPreProcessor(0.0f, 1.0f));
    _release_parameter = configuration.controller->register_float_parameter("release", "Release", 0.0f, new FloatParameterPreProcessor(0.0f, 10.0f));

    for (auto& voice : _voices)
    {
        voice.set_samplerate(configuration.sample_rate);
    }
    if (load_sample_file(SAMPLE_FILE) != 0)
    {
        MIND_LOG_ERROR("Sample file not found");
        return StompBoxStatus::ERROR;
    }
    return StompBoxStatus::OK;
}


void SamplePlayerPlugin::process_event(BaseMindEvent* event)
{
    switch (event->type())
    {
        case MindEventType::NOTE_ON:
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
        case MindEventType::NOTE_OFF:
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
        voice.render(*out_buffer);
    }
    out_buffer->add_with_gain(_buffer, gain);
}


int SamplePlayerPlugin::load_sample_file(const std::string& file_name)
{
    SNDFILE*    sample_file;
    SF_INFO     soundfile_info = {};

    if (! (sample_file = sf_open(file_name.c_str(), SFM_READ, &soundfile_info)) )
    {
        return -1;
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
    return 0;
}


}// namespace sample_player_plugin
}// namespace sushi