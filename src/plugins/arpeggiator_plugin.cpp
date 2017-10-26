#include "plugins/arpeggiator_plugin.h"
#include "logging.h"

namespace sushi {
namespace arpeggiator_plugin {

MIND_GET_LOGGER;

constexpr float SECONDS_IN_MINUTE = 60.0f;
constexpr float EIGHTH_NOTE = 0.125f;

ArpeggiatorPlugin::ArpeggiatorPlugin()
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _tempo_parameter  = register_float_parameter("tempo", "Tempo", 120.0f, 20.0f, 1000.0f, new FloatParameterPreProcessor(20.0f, 1000.0f));
    _range_parameter  = register_int_parameter("range", "Range", 2, 1, 5, new IntParameterPreProcessor(1, 5));
    assert(_tempo_parameter && _range_parameter);
    _max_input_channels = 0;
    _max_output_channels = 0;
}


ProcessorReturnCode ArpeggiatorPlugin::init(float sample_rate)
{
    _sample_rate = sample_rate;
    return ProcessorReturnCode::OK;
}

void ArpeggiatorPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
}

void ArpeggiatorPlugin::set_bypassed(bool bypassed)
{
    Processor::set_bypassed(bypassed);
}

void ArpeggiatorPlugin::process_event(RtEvent event)
{
    switch (event.type())
    {
        case RtEventType::NOTE_ON:
        {
            auto typed_event = event.keyboard_event();
            _arp.add_note(typed_event->note());
            break;
        }

        case RtEventType::NOTE_OFF:
        {
            auto typed_event = event.keyboard_event();
            _arp.remove_note(typed_event->note());
            break;
        }

        case RtEventType::INT_PARAMETER_CHANGE:
        case RtEventType::FLOAT_PARAMETER_CHANGE:
        {
            auto typed_event = event.parameter_change_event();
            if (typed_event->param_id() == _range_parameter->descriptor()->id())
            {
                _arp.set_range(static_cast<int>(typed_event->value()));
            }
            /* Intentional fall through */
        }
        default:
            InternalPlugin::process_event(event);
    }
}

void ArpeggiatorPlugin::process_audio(const ChunkSampleBuffer&/*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/)
{
    float period = samples_per_note(EIGHTH_NOTE, _tempo_parameter->value(), _sample_rate);
    _sample_counter += AUDIO_CHUNK_SIZE;
    while (_sample_counter >= period)
    {
        _sample_counter -= period;
        int offset = std::max(static_cast<int>(_sample_counter), AUDIO_CHUNK_SIZE -1);
        RtEvent e = RtEvent::make_note_off_event(this->id(), offset, _current_note, 1.0f);
        output_event(e);

        _current_note = _arp.next_note();
        e = RtEvent::make_note_on_event(this->id(), offset, _current_note, 1.0f);
        output_event(e);
    }
}

float samples_per_note(float note_fraction, float tempo, float samplerate)
{
    return 4.0f * note_fraction * samplerate / tempo * SECONDS_IN_MINUTE;
}

Arpeggiator::Arpeggiator()
{
    /* Reserving a maximum capacity and never exceeding it makes std::vector
     * safe to use in a real-time environment */
    _notes.reserve(MAX_ARP_NOTES);
    _notes.push_back(START_NOTE);
}

void Arpeggiator::add_note(int note)
{
    if (_hold)
    {
        _hold = false;
        _notes.clear();
    }
    if (_notes.size() < _notes.capacity())
    {
        _notes.push_back(note);
    }
}

void Arpeggiator::remove_note(int note)
{
    if (_notes.size() <= 1)
    {
        _hold = true;
        return;
    }
    for (auto i = _notes.begin(); i != _notes.end(); ++i)
    {
        if (*i == note)
        {
            _notes.erase(i);
            return;
        }
    }
}

void Arpeggiator::set_range(int range)
{
    _range = range;
}

int Arpeggiator::next_note()
{
    if (++_note_idx >= static_cast<int>(_notes.size()))
    {
        _note_idx = 0;
        if (++_octave_idx >= _range)
        {
            _octave_idx = 0;
        }
    }
    return _notes[_note_idx] + _octave_idx * 12;
}
}// namespace sample_player_plugin
}// namespace sushi