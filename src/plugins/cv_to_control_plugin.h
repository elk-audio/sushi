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
 * @brief Adapter plugin to convert cv/gate information to note on and note
 *        off messages to enable cv/gate control of synthesizer plugins.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CV_TO_CONTROL_PLUGIN_H
#define SUSHI_CV_TO_CONTROL_PLUGIN_H

#include <array>
#include <vector>

#include "library/internal_plugin.h"
#include "library/constants.h"
#include "library/rt_event_fifo.h"

namespace sushi {
namespace cv_to_control_plugin {

constexpr int MAX_CV_VOICES = MAX_ENGINE_CV_IO_PORTS;

class CvToControlPlugin : public InternalPlugin, public UidHelper<CvToControlPlugin>
{
public:
    CvToControlPlugin(HostControl host_control);

    ~CvToControlPlugin() {}

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& /*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/) override;

    static std::string_view static_uid();

private:

    void _send_deferred_events(int channel);
    void _process_cv_signals(int polyphony, int channel, int tune, bool send_velocity, bool send_pitch_bend);
    void _process_gate_changes(int polyphony, int channel, int tune, bool send_velocity, bool send_pitch_bend);

    struct ControlVoice
    {
        bool active{false};
        int  note{0};
    };

    BoolParameterValue* _pitch_bend_mode_parameter;
    BoolParameterValue* _velocity_mode_parameter;
    IntParameterValue*  _channel_parameter;
    IntParameterValue*  _coarse_tune_parameter;
    IntParameterValue*  _polyphony_parameter;

    std::array<FloatParameterValue*, MAX_CV_VOICES> _pitch_parameters;
    std::array<FloatParameterValue*, MAX_CV_VOICES> _velocity_parameters;

    std::array<ControlVoice, MAX_CV_VOICES>         _voices;
    std::vector<int>                                _deferred_note_offs;
    RtEventFifo<MAX_ENGINE_GATE_PORTS>              _gate_events;
};

std::pair<int, float> cv_to_pitch(float value);

}// namespace cv_to_control_plugin
}// namespace sushi
#endif //SUSHI_CV_TO_CONTROL_PLUGIN_H
