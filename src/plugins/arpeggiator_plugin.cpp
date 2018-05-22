#include "plugins/arpeggiator_plugin.h"
#include "logging.h"

namespace sushi {
namespace arpeggiator_plugin {

MIND_GET_LOGGER;

constexpr float SECONDS_IN_MINUTE = 60.0f;
constexpr float MULTIPLIER_8TH_NOTE = 2.0f;
constexpr int   OCTAVE = 12;

ArpeggiatorPlugin::ArpeggiatorPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _range_parameter  = register_int_parameter("range", "Range", 2, 1, 5, new IntParameterPreProcessor(1, 5));
    assert(_range_parameter);
    _max_input_channels = 0;
    _max_output_channels = 0;
    _last_note_beat = _host_control.transport()->current_beats();
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
            [[fallthrough]];
        }
        default:
            InternalPlugin::process_event(event);
    }
}

void ArpeggiatorPlugin::process_audio(const ChunkSampleBuffer&/*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/)
{
    double beat = _host_control.transport()->current_beats();
    double last_beat_this_chunk = _host_control.transport()->current_beats(AUDIO_CHUNK_SIZE);
    double beat_period = last_beat_this_chunk - beat;
    auto notes_this_chunk = MULTIPLIER_8TH_NOTE * (last_beat_this_chunk -_last_note_beat);

    while (notes_this_chunk > 1.0)
    {
        double next_note_beat = _last_note_beat + 1 / MULTIPLIER_8TH_NOTE;
        auto fraction = next_note_beat - beat - std::floor(next_note_beat - beat);
        _last_note_beat = next_note_beat;
        int offset = 0;
        if (fraction > 0)
        {
            /* If fraction is not positive, then there was a missed beat in an underrun */
            offset = std::min(static_cast<int>(std::round(AUDIO_CHUNK_SIZE * fraction / beat_period)), AUDIO_CHUNK_SIZE -1);
        }

        RtEvent note_off = RtEvent::make_note_off_event(this->id(), offset, _current_note, 1.0f);
        _current_note = _arp.next_note();
        RtEvent note_on = RtEvent::make_note_on_event(this->id(), offset, _current_note, 1.0f);
        output_event(note_off);
        output_event(note_on);

        notes_this_chunk = MULTIPLIER_8TH_NOTE * (last_beat_this_chunk -_last_note_beat);
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
    return _notes[_note_idx] + _octave_idx * OCTAVE;
}
}// namespace sample_player_plugin
}// namespace sushi