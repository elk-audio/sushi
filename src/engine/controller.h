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
 * @Brief Controller object for external control of sushi
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "control_interface.h"
#include "base_event_dispatcher.h"
#include "transport.h"
#include "library/base_performance_timer.h"

#ifndef SUSHI_CONTROLLER_H
#define SUSHI_CONTROLLER_H

namespace sushi {

namespace engine {class BaseEngine;}

class Controller : public ext::SushiControl
{
public:
    Controller(engine::BaseEngine* engine);

    ~Controller();

    float                                               get_samplerate() const override;
    ext::PlayingMode                                    get_playing_mode() const override;
    void                                                set_playing_mode(ext::PlayingMode playing_mode) override;
    ext::SyncMode                                       get_sync_mode() const override;
    void                                                set_sync_mode(ext::SyncMode sync_mode) override;
    float                                               get_tempo() const override;
    ext::ControlStatus                                  set_tempo(float tempo) override;
    ext::TimeSignature                                  get_time_signature() const override;
    ext::ControlStatus                                  set_time_signature(ext::TimeSignature signature) override;
    bool                                                get_timing_statistics_enabled() override;
    void                                                set_timing_statistics_enabled(bool enabled) const override;
    std::vector<ext::TrackInfo>                         get_tracks() const override;

    ext::ControlStatus                                  send_note_on(int track_id, int channel, int note, float velocity) override;
    ext::ControlStatus                                  send_note_off(int track_id, int channel, int note, float velocity) override;
    ext::ControlStatus                                  send_note_aftertouch(int track_id, int channel, int note, float value) override;
    ext::ControlStatus                                  send_aftertouch(int track_id, int channel, float value) override;
    ext::ControlStatus                                  send_pitch_bend(int track_id, int channel, float value) override;
    ext::ControlStatus                                  send_modulation(int track_id, int channel, float value) override;

    std::pair<ext::ControlStatus, ext::CpuTimings>      get_engine_timings() const override;
    std::pair<ext::ControlStatus, ext::CpuTimings>      get_track_timings(int track_id) const override;
    std::pair<ext::ControlStatus, ext::CpuTimings>      get_processor_timings(int processor_id) const override;
    ext::ControlStatus                                  reset_all_timings() override;
    ext::ControlStatus                                  reset_track_timings(int track_id) override;
    ext::ControlStatus                                  reset_processor_timings(int processor_id) override;

    std::pair<ext::ControlStatus, int>                  get_track_id(const std::string& track_name) const override;
    std::pair<ext::ControlStatus, ext::TrackInfo>       get_track_info(int track_id) const override;
    std::pair<ext::ControlStatus, std::vector<ext::ProcessorInfo>> get_track_processors(int track_id) const override;
    std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> get_track_parameters(int processor_id) const override;

    std::pair<ext::ControlStatus, int>                  get_processor_id(const std::string& processor_name) const override;
    std::pair<ext::ControlStatus, ext::ProcessorInfo>   get_processor_info(int processor_id) const override;
    std::pair<ext::ControlStatus, bool>                 get_processor_bypass_state(int processor_id) const override;
    ext::ControlStatus                                  set_processor_bypass_state(int processor_id, bool bypass_enabled) override;
    std::pair<ext::ControlStatus, int>                  get_processor_current_program(int processor_id) const override;
    std::pair<ext::ControlStatus, std::string>          get_processor_current_program_name(int processor_id) const override;
    std::pair<ext::ControlStatus, std::string>          get_processor_program_name(int processor_id, int program_id) const override;
    std::pair<ext::ControlStatus, std::vector<std::string>> get_processor_programs(int processor_id) const override ;
    ext::ControlStatus                                  set_processor_program(int processor_id, int program_id) override;
    std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> get_processor_parameters(int processor_id) const override;

    std::pair<ext::ControlStatus, int>                  get_parameter_id(int processor_id, const std::string& parameter) const override;
    std::pair<ext::ControlStatus, ext::ParameterInfo>   get_parameter_info(int processor_id, int parameter_id) const override;
    std::pair<ext::ControlStatus, float>                get_parameter_value(int processor_id, int parameter_id) const override;
    std::pair<ext::ControlStatus, float>                get_parameter_value_normalised(int processor_id, int parameter_id) const override;
    std::pair<ext::ControlStatus, std::string>          get_parameter_value_as_string(int processor_id, int parameter_id) const override;
    std::pair<ext::ControlStatus, std::string>          get_string_property_value(int processor_id, int parameter_id) const override;
    ext::ControlStatus                                  set_parameter_value(int processor_id, int parameter_id, float value) override;
    ext::ControlStatus                                  set_parameter_value_normalised(int processor_id, int parameter_id, float value) override;
    ext::ControlStatus                                  set_string_property_value(int processor_id, int parameter_id, const std::string& value) override;

protected:
    std::pair<ext::ControlStatus, ext::CpuTimings> _get_timings(int node) const;

    engine::BaseEngine*                 _engine;
    dispatcher::BaseEventDispatcher*    _event_dispatcher;
    performance::BasePerformanceTimer*  _performance_timer;
};

} //namespace sushi
#endif //SUSHI_CONTROLLER_H
