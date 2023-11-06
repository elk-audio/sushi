/*
* Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
*
* SUSHI is free software: you can redistribute it and/or modify it under the terms of
* the GNU Affero General Public License as published by the Free Software Foundation,
* either version 3 of the License, or (at your option) any later version.
*
* SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License along with
* SUSHI. If not, see http://www.gnu.org/licenses/
*/

#include "real_time_controller.h"

#include "audio_frontends/reactive_frontend.h"
#include "control_frontends/reactive_midi_frontend.h"

#include "engine/audio_engine.h"
#include "engine/controller/controller_common.h"
#include "engine/event_timer.h"

namespace sushi::internal
{

RealTimeController::RealTimeController(audio_frontend::ReactiveFrontend* audio_frontend,
                                       midi_frontend::ReactiveMidiFrontend* midi_frontend,
                                       engine::Transport* transport) : _audio_frontend(audio_frontend),
                                                                       _midi_frontend(midi_frontend),
                                                                       _transport(transport)
{
}

RealTimeController::~RealTimeController()
{
}

void RealTimeController::set_tempo(float tempo)
{
    if (_tempo != tempo)
    {
        _transport->set_tempo(tempo, false); // update_via_event
        _tempo = tempo;
    }
}

void RealTimeController::set_time_signature(control::TimeSignature time_signature)
{
    auto internal_time_signature = engine::controller_impl::to_internal(time_signature);

    if (_time_signature != internal_time_signature)
    {
        _transport->set_time_signature(internal_time_signature, false); // update_via_event

        _time_signature = internal_time_signature;
    }
}

void RealTimeController::set_playing_mode(control::PlayingMode mode)
{
    auto internal_playing_mode = engine::controller_impl::to_internal(mode);

    if (_playing_mode != mode)
    {
        _transport->set_playing_mode(internal_playing_mode,
                                     false); // update_via_event
        _playing_mode = mode;
    }
}

bool RealTimeController::set_current_beats(double beat_count)
{
    if (_transport->position_source() == PositionSource::EXTERNAL)
    {
        _transport->set_current_beats(beat_count);
        return true;
    }

    return false;
}

bool RealTimeController::set_current_bar_beats(double bar_beat_count)
{
    if (_transport->position_source() == PositionSource::EXTERNAL)
    {
        _transport->set_current_bar_beats(bar_beat_count);
        return true;
    }

    return false;
}

void RealTimeController::set_position_source(TransportPositionSource ps)
{
    if (ps == sushi::TransportPositionSource::CALCULATED)
    {
        _transport->set_position_source(PositionSource::CALCULATED);
    }
    else
    {
        _transport->set_position_source(PositionSource::EXTERNAL);
    }
}

void RealTimeController::process_audio(ChunkSampleBuffer& in_buffer,
                                       ChunkSampleBuffer& out_buffer,
                                       Time timestamp)
{
    _audio_frontend->process_audio(in_buffer,
                                   out_buffer,
                                   _samples_since_start,
                                   timestamp);
}

void RealTimeController::receive_midi(int input, MidiDataByte data, Time timestamp)
{
    _midi_frontend->receive_midi(input, data, timestamp);
}

void RealTimeController::set_midi_callback(ReactiveMidiCallback&& callback)
{
    _midi_frontend->set_callback(std::move(callback));
}

sushi::Time RealTimeController::calculate_timestamp_from_start(float sample_rate) const
{
    uint64_t micros = _samples_since_start * 1'000'000.0 / sample_rate;
    auto timestamp = std::chrono::microseconds(micros);
    return timestamp;
}

void RealTimeController::increment_samples_since_start(uint64_t sample_count, Time)
{
    _samples_since_start += sample_count;
}

} // end namespace sushi