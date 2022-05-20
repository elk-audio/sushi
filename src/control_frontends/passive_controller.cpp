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

#include "include/sushi/passive_controller.h"

#include "control_frontends/passive_midi_frontend.h"
#include "audio_frontends/passive_frontend.h"

#include "engine/audio_engine.h"
#include "engine/controller/controller_common.h"
#include "engine/event_timer.h"

namespace sushi
{

PassiveController::PassiveController()
{
    _event_timer = std::make_unique<event_timer::EventTimer>(SUSHI_SAMPLE_RATE_DEFAULT);
}

PassiveController::~PassiveController()
{
    _sushi->exit();
}

void PassiveController::init(SushiOptions& options)
{
    _sushi = std::make_unique<sushi::Sushi>();

    sushi::init_logger(options); // This can only be called once.

    // Overriding whatever frontend settings may or may not have been set.
    // This also causes the MIDI frontend to be set to NullMidiFrontend in Sushi::_set_up_control.
    options.frontend_type = sushi::FrontendType::PASSIVE;

    auto sushiInitStatus = _sushi->init(options);

    if (sushiInitStatus != sushi::InitStatus::OK)
    {
        // TODO: Throw? Exit?
        assert(false);
    }

    _audio_frontend = _sushi->audio_frontend();
    _midi_frontend = _sushi->midi_frontend();
    _transport = _sushi->audio_engine()->transport();

    _sushi->start();
}

void PassiveController::set_tempo(float tempo)
{
    // TODO: This works, but, it triggers the non-rt-safe Ableton Link event code.
    //  We want Ableton Link to be disabled anyway when Sushi is passive!
    //  So that work is a separate story (AUD-460).
    if (_tempo != tempo)
    {
        _transport->set_tempo(tempo,
                              false); // update_via_event
        _tempo = tempo;
    }
}

void PassiveController::set_time_signature(ext::TimeSignature time_signature)
{
    auto internal_time_signature = engine::to_internal(time_signature);

    if (_time_signature != internal_time_signature)
    {
        _transport->set_time_signature(internal_time_signature,
                                       false); // update_via_event
        _time_signature = internal_time_signature;
    }
}

void PassiveController::set_playing_mode(ext::PlayingMode mode)
{
    auto internal_playing_mode = engine::to_internal(mode);

    if (_playing_mode != mode)
    {
        _transport->set_playing_mode(internal_playing_mode,
                                     false); // update_via_event
        _playing_mode = mode;
    }
}

void PassiveController::set_beat_count(double beat_count)
{
    if (_transport->position_source() == sushi::PositionSource::EXTERNAL)
    {
        _transport->set_beat_count(beat_count);
    }
    else
    {
        assert(false);
    }
}

void PassiveController::set_position_source(TransportPositionSource ps)
{
    if (ps == sushi::TransportPositionSource::CALCULATED)
    {
        _transport->set_position_source(sushi::PositionSource::CALCULATED);
    }
    else
    {
        _transport->set_position_source(sushi::PositionSource::EXTERNAL);
    }
}

void PassiveController::process_audio(int channel_count,
                                       int64_t sample_count,
                                       Time timestamp)
{
    _audio_frontend->process_audio(channel_count,
                                   sample_count,
                                   timestamp);
}

void PassiveController::receive_midi(int input, MidiDataByte data, Time timestamp)
{
    _midi_frontend->receive_midi(input, data, timestamp);
}

void PassiveController::set_midi_callback(PassiveMidiCallback&& callback)
{
    _midi_frontend->set_callback(std::move(callback));
}

ChunkSampleBuffer& PassiveController::in_buffer()
{
    return _audio_frontend->in_buffer();
}

ChunkSampleBuffer& PassiveController::out_buffer()
{
    return _audio_frontend->out_buffer();
}

void PassiveController::set_sample_rate(double sample_rate)
{
    _sample_rate = sample_rate;

    _sushi->set_sample_rate(sample_rate);

    _event_timer->set_sample_rate(sample_rate);
}

double PassiveController::sample_rate() const
{
    return _sample_rate;
}

void PassiveController::set_incoming_time(Time timestamp)
{
    _event_timer->set_incoming_time(timestamp);
}

void PassiveController::set_outgoing_time(Time timestamp)
{
    _event_timer->set_outgoing_time(timestamp);
}

sushi::Time PassiveController::timestamp_from_start() const
{
    uint64_t micros = _samples_since_start * 1'000'000.0 / _sample_rate;
    return std::chrono::microseconds(micros);
}

uint64_t PassiveController::samples_since_start() const
{
    return _samples_since_start;
}

void PassiveController::increment_samples_since_start(uint64_t amount)
{
    _samples_since_start += amount;
}

Time PassiveController::real_time_from_sample_offset(int offset)
{
    return _event_timer->real_time_from_sample_offset(offset);
}

std::pair<bool, int> PassiveController::sample_offset_from_realtime(Time timestamp)
{
    return _event_timer->sample_offset_from_realtime(timestamp);
}

sushi::Time PassiveController::timestamp_from_clock()
{
    auto time = std::chrono::steady_clock::now().time_since_epoch();

    if (!_is_start_time_set)
    {
        _is_start_time_set = true;
        _start_time = time;
    }

    auto timestamp = std::chrono::duration_cast<sushi::Time>(time - _start_time);

    return timestamp;
}

} // namespace sushi