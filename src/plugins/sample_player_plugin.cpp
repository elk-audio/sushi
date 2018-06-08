#include <cassert>
#include <sndfile.h>

#include "sample_player_plugin.h"
#include "logging.h"

namespace sushi {
namespace sample_player_plugin {

MIND_GET_LOGGER;

SamplePlayerPlugin::SamplePlayerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _volume_parameter  = register_float_parameter("volume", "Volume", 0.0f, -120.0f, 36.0f, new dBToLinPreProcessor(-120.0f, 36.0f));
    _attack_parameter  = register_float_parameter("attack", "Attack", 0.0f, 0.0f, 10.0f, new FloatParameterPreProcessor(0.0f, 10.0f));
    _decay_parameter   = register_float_parameter("decay", "Decay", 0.0f, 0.0f, 10.0f, new FloatParameterPreProcessor(0.0f, 10.0f));
    _sustain_parameter = register_float_parameter("sustain", "Sustain", 1.0f, 0.0f, 1.0f, new FloatParameterPreProcessor(0.0f, 1.0f));
    _release_parameter = register_float_parameter("release", "Release", 0.0f, 0.0f, 10.0f, new FloatParameterPreProcessor(0.0f, 10.0f));
    [[maybe_unused]] bool str_pr_ok = register_string_property("sample_file", "Sample File");
    assert(_volume_parameter && _attack_parameter && _decay_parameter && _sustain_parameter && _release_parameter && str_pr_ok);
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

void SamplePlayerPlugin::set_bypassed(bool bypassed)
{
    // Kill all voices in bypass so we dont have any hanging notes when turning back on
    if (bypassed)
    {
        for (auto &voice : _voices)
        {
            voice.note_off(1.0f, 0);
        }
    }
    Processor::set_bypassed(bypassed);
}

SamplePlayerPlugin::~SamplePlayerPlugin()
{
    delete _sample_buffer;
    delete _sample_file_property;
}

void SamplePlayerPlugin::process_event(RtEvent event)
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
            MIND_LOG_DEBUG("Sample Player: note ON, num. {}, vel. {}",
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
            MIND_LOG_DEBUG("Sample Player: note OFF, num. {}, vel. {}",
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
        case RtEventType::STRING_PROPERTY_CHANGE:
        {
            /* Currently there is only 1 string parameter and it's for changing the sample
             * file, hence no need to check the parameter id */
            auto typed_event = event.string_parameter_change_event();
            for (auto& voice : _voices)
            {
                voice.note_off(1.0f, 0);
            }
            _sample_file_property = typed_event->value();
            /* Schedule a non-rt callback to handle sample loading */
            auto e = RtEvent::make_async_work_event(&SamplePlayerPlugin::non_rt_callback, this->id(), this);
            _pending_event_id = e.async_work_event()->event_id();
            output_event(e);
            break;
        }
        case RtEventType::ASYNC_WORK_NOTIFICATION:
        {
            auto typed_event = event.async_work_completion_event();
            if (typed_event->sending_event_id() == _pending_event_id &&
                typed_event->return_status() == SampleChangeStatus::SUCCESS)
            {
                float* old_sample = _sample_buffer;
                _sample_buffer = reinterpret_cast<float*>(_pending_sample.data);
                _sample.set_sample(_sample_buffer, _pending_sample.size / sizeof(float));
                /* Delete the old sample data outside the rt thread */
                BlobData data{0, reinterpret_cast<uint8_t*>(old_sample)};
                auto delete_event = RtEvent::make_delete_blob_event(data);
                output_event(delete_event);
            }
            break;
        }

        default:
            InternalPlugin::process_event(event);
            break;
    }
}


void SamplePlayerPlugin::process_audio(const ChunkSampleBuffer& /* in_buffer */, ChunkSampleBuffer &out_buffer)
{
    float gain = _volume_parameter->value();
    float attack = _attack_parameter->value();
    float decay = _decay_parameter->value();
    float sustain = _sustain_parameter->value();
    float release = _release_parameter->value();

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

BlobData SamplePlayerPlugin::load_sample_file(const std::string &file_name)
{
    SNDFILE*    sample_file;
    SF_INFO     soundfile_info = {};
    if (! (sample_file = sf_open(file_name.c_str(), SFM_READ, &soundfile_info)) )
    {
        MIND_LOG_ERROR("Failed to open sample file: {}", file_name);
        return {0,0};
    }
    assert(soundfile_info.channels == 1);
    float* sample_buffer = new float[soundfile_info.frames];
    int samples = sf_readf_float(sample_file, sample_buffer, soundfile_info.frames);
    assert(samples == soundfile_info.frames);
    sf_close(sample_file);

    return BlobData{static_cast<int>(samples * sizeof(float)), reinterpret_cast<uint8_t*>(sample_buffer)};
}

int SamplePlayerPlugin::_non_rt_callback(EventId id)
{
    if (id == _pending_event_id)
    {
        /* Note that this doesn't handle multiple requests at once, several outstanding work
         * requests can leak the address string */
        auto sample_data = load_sample_file(*_sample_file_property);
        delete _sample_file_property;
        _sample_file_property = nullptr;
        if (sample_data.size > 0)
        {
            _pending_sample = sample_data;
            MIND_LOG_INFO("SamplePlayer: Successfully loaded sample data");
            return SampleChangeStatus::SUCCESS;
        }
        return SampleChangeStatus::FAILURE;
    }
    else
    {
        MIND_LOG_WARNING("Sampleplayer: EventId of non-rt callback didn't match, {} vs {}", id, _pending_event_id);
        return SampleChangeStatus::FAILURE;
    }
}


}// namespace sample_player_plugin
}// namespace sushi
