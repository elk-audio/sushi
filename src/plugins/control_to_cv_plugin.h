/**
 * @brief Adapter plugin to convert cv/gate information to note on and note
 *        off messages to enable cv/gate control of synthesizer plugins.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_CONTROL_TO_CV_PLUGIN_H
#define SUSHI_CONTROL_TO_CV_PLUGIN_H

#include <array>
#include <vector>

#include "library/internal_plugin.h"
#include "library/constants.h"
#include "library/rt_event_fifo.h"

namespace sushi {
namespace control_to_cv_plugin {

constexpr int MAX_CV_VOICES = MAX_ENGINE_CV_IO_PORTS;

class ControlToCvPlugin : public InternalPlugin
{
public:
    ControlToCvPlugin(HostControl host_control);

    ~ControlToCvPlugin() {}

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& /*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/) override;

private:
    struct ControlVoice
    {
        bool  active{false};
        int   note{0};
        float velocity{0};
    };

    void _send_deferred_events();
    void _parse_events(bool retrigger, int polyphony);
    void _send_cv_signals(float tune_offset, int polyphony, bool send_velocity, bool send_modulation);
    int get_free_voice_id(int polyphony);

    BoolParameterValue*   _send_velocity_parameter;
    BoolParameterValue*   _send_modulation_parameter;
    BoolParameterValue*   _retrigger_mode_parameter;
    IntParameterValue*    _coarse_tune_parameter;
    FloatParameterValue*  _fine_tune_parameter;
    IntParameterValue*    _polyphony_parameter;

    FloatParameterValue* _modulation_parameter;
    std::array<FloatParameterValue*, MAX_CV_VOICES> _pitch_parameters;
    std::array<FloatParameterValue*, MAX_CV_VOICES> _velocity_parameters;

    float _pitch_bend_value{0};
    float _modulation_value{0};

    int                                             _last_voice{0};
    std::array<ControlVoice, MAX_CV_VOICES>         _voices;
    RtEventFifo<MAX_ENGINE_GATE_PORTS>              _kb_events;
    SimpleFifo<int, MAX_ENGINE_GATE_PORTS>          _deferred_gate_highs;
};

float pitch_to_cv(float value);

}// namespace control_to_cv_plugin
}// namespace sushi
#endif //SUSHI_CONTROL_TO_CV_PLUGIN_H
