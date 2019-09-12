#include <algorithm>
#include <cmath>

#include "plugins/control_to_cv_plugin.h"

namespace sushi {
namespace control_to_cv_plugin {

static const std::string DEFAULT_NAME = "sushi.testing.control_to_cv";
static const std::string DEFAULT_LABEL = "Keyboard control to CV adapter";
constexpr int TUNE_RANGE = 24;
constexpr float PITCH_BEND_RANGE = 12.0f;

ControlToCvPlugin::ControlToCvPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);

    _send_velocity_parameter = register_bool_parameter("send_velocity", "Send Velocity", false);
    _send_modulation_parameter = register_bool_parameter("send_modulation", "Send Modulation", false);
    _retrigger_mode_parameter = register_bool_parameter("retrigger_enabled", "Retrigger enabled", false);
    _coarse_tune_parameter  = register_int_parameter("tune", "Tune", 0, -TUNE_RANGE, TUNE_RANGE, new IntParameterPreProcessor(-24, 24));
    _fine_tune_parameter  = register_float_parameter("fine_tune", "Fine Tune", 0, -1, 1, new FloatParameterPreProcessor(-1, 1));
    _polyphony_parameter  = register_int_parameter("polyphony", "Polyphony", 1, 1, MAX_CV_VOICES,
                                                 new IntParameterPreProcessor(1, MAX_CV_VOICES));
    _modulation_parameter  = register_float_parameter("modulation", "Modulation", 0, -1, 1, new FloatParameterPreProcessor(-1, 1));

    assert(_send_velocity_parameter && _send_modulation_parameter && _coarse_tune_parameter &&
                                             _polyphony_parameter && _modulation_parameter);

    for (int i = 0; i < MAX_CV_VOICES; ++i)
    {
        auto i_str = std::to_string(i + 1);
        _pitch_parameters[i] = register_float_parameter("pitch_" + i_str, "Pitch " + i_str, 0, 0, 1, new FloatParameterPreProcessor(0, 1));
        _velocity_parameters[i] = register_float_parameter("velocity_" + i_str, "Velocity " + i_str, 0.5, 0, 1, new FloatParameterPreProcessor(0, 1));
        assert(_pitch_parameters[i] && _velocity_parameters[i]);
    }
    _max_input_channels = 0;
    _max_output_channels = 0;
}

ProcessorReturnCode ControlToCvPlugin::init(float sample_rate)
{
    return Processor::init(sample_rate);
}

void ControlToCvPlugin::configure(float sample_rate)
{
    Processor::configure(sample_rate);
}

void ControlToCvPlugin::process_event(const RtEvent& event)
{
    if (is_keyboard_event(event))
    {
        _kb_events.push(event);
        return;
    }
    InternalPlugin::process_event(event);
}

void ControlToCvPlugin::process_audio(const ChunkSampleBuffer&  /*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/)
{
    if (_bypassed == true)
    {
        _kb_events.clear();
        return;
    }

    bool send_velocity = _send_velocity_parameter->value();
    bool send_modulation = _send_modulation_parameter->value();
    bool retrigger_mode = _retrigger_mode_parameter->value();
    int  coarse_tune = _coarse_tune_parameter->value();
    float fine_tune = _fine_tune_parameter->value();
    int  polyphony = _polyphony_parameter->value();

    float tune_offset = coarse_tune + fine_tune;

    while (_deferred_events.empty() == false)
    {
        auto event = _deferred_events.pop();
        this->output_event(event);
    }
    _deferred_events.clear();

    while (_kb_events.empty() == false)
    {
        auto event = _kb_events.pop();
        switch (event.type())
        {
            case RtEventType::NOTE_ON:
            {
                auto typed_event = event.keyboard_event();
                int voice_id = get_free_voice_id(polyphony);
                auto& voice = _voices[voice_id];
                if (retrigger_mode && voice.active)
                {
                    // Send the gate low event now, and send the gate high event to the next buffer
                    output_event(RtEvent::make_gate_event(this->id(), 0, voice_id, false));
                    _deferred_events.push(RtEvent::make_gate_event(this->id(), 0, voice_id, true));
                }
                else if (voice.active == false)
                {
                    output_event(RtEvent::make_gate_event(this->id(), 0, voice_id, true));
                }
                voice.active = true;
                voice.note = typed_event->note();
                voice.velocity = typed_event->velocity();
                break;
            }

            case RtEventType::NOTE_OFF:
            {
                auto typed_event = event.keyboard_event();
                for (int i = 0; i < polyphony; ++i)
                {
                    auto& voice = _voices[i];
                    if (voice.note == typed_event->note() && voice.active)
                    {
                        output_event(RtEvent::make_gate_event(this->id(), 0, i, false));
                        voice.active = false; // TODO - should we handle release velocity, should it be optional?
                    }
                }
                break;
            }

            case RtEventType::PITCH_BEND:
            {
                auto typed_event = event.keyboard_common_event();
                _pitch_bend_value = typed_event->value() * PITCH_BEND_RANGE;
                break;
            }
            case RtEventType::MODULATION:
            {
                auto typed_event = event.keyboard_common_event();
                _modulation_value = typed_event->value();
                break;
            }

            default:
                break;
        }
    }
    _kb_events.clear();

    // As notes have a non-zero decay, pitch matters even if gate is off, hence always send pitch on all notes
    for (int i = 0; i < polyphony; ++i)
    {
        set_parameter_and_notify(_pitch_parameters[i], pitch_to_cv(_voices[i].note + _pitch_bend_value + tune_offset));
    }
    if (send_velocity)
    {
        for (int i = 0; i < polyphony; ++i)
        {
            set_parameter_and_notify(_velocity_parameters[i], _voices[i].velocity);
        }
    }
    if (send_modulation)
    {
        set_parameter_and_notify(_modulation_parameter, _modulation_value);
    }
}

int ControlToCvPlugin::get_free_voice_id(int polyphony)
{
    assert(polyphony <= MAX_CV_VOICES);
    int voice_id = 0;
    if (polyphony > 1)
    {
        while (voice_id < polyphony && _voices[voice_id].active)
        {
            voice_id++;
        }
        if (voice_id >= polyphony) // We need to steal an active note, pick the last
        {
            voice_id = _last_voice;
            _last_voice = (_last_voice + 1) % polyphony;
        }
        else
        {
            _last_voice = 0;
        }
    }
    return voice_id;
}

float pitch_to_cv(float value)
{
    // TODO - this need a lot of tuning, or maybe that tuning should be done someplace else in the code?
    // Currently just assuming [0, 1] covers a 10 octave linear range.
    return std::clamp(value / 120.f, 0.0f, 1.0f);
}
}// namespace control_to_cv_plugin
}// namespace sushi