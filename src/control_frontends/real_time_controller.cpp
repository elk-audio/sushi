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

#include "include/sushi/real_time_controller.h"
#include "include/sushi/sushi.h"

#include "control_frontends/passive_midi_frontend.h"
#include "audio_frontends/passive_frontend.h"

#include "engine/audio_engine.h"
#include "engine/controller/controller_common.h"
#include "engine/event_timer.h"

namespace sushi
{

RealTimeController::RealTimeController(Sushi* sushi)
{
    _sushi = sushi;

    _event_timer = std::make_unique<event_timer::EventTimer>(SUSHI_SAMPLE_RATE_DEFAULT);
}

RealTimeController::~RealTimeController() = default;

void RealTimeController::init()
{
    _audio_frontend = _sushi->audio_frontend();
    _midi_frontend = _sushi->midi_frontend();
}

void RealTimeController::set_tempo(float tempo)
{
    // TODO AUD-426: Make RT safe.
    _sushi->audio_engine()->set_tempo(tempo);
}

void RealTimeController::set_time_signature(sushi::ext::TimeSignature time_signature)
{
    // TODO AUD-426: Make RT safe.
    _sushi->audio_engine()->set_time_signature(engine::to_internal(time_signature));
}

void RealTimeController::set_playing_mode(sushi::ext::PlayingMode mode)
{
    // TODO AUD-426: Make RT safe.
    _sushi->audio_engine()->set_transport_mode(engine::to_internal(mode));
}

void RealTimeController::set_sync_mode(ext::SyncMode sync_mode)
{
    // TODO AUD-426: populate once above are done.
}

void RealTimeController::set_beat_time(float beat_time)
{
    // TODO AUD-426: populate once above are done.
}

void RealTimeController::process_audio(int channel_count,
                                       int64_t sample_count,
                                       Time timestamp)
{
    _audio_frontend->process_audio(&_in_buffer, &_out_buffer, channel_count, sample_count, timestamp);
}

void RealTimeController::receive_midi(int input, MidiDataByte data, Time timestamp)
{
    _midi_frontend->receive_midi(input, data, timestamp);
}

void RealTimeController::set_midi_callback(PassiveMidiCallback&& callback)
{
    _midi_frontend->set_callback(std::move(callback));
}

ChunkSampleBuffer& RealTimeController::in_buffer()
{
    return _in_buffer;
}

ChunkSampleBuffer& RealTimeController::out_buffer()
{
    return _out_buffer;
}

void RealTimeController::set_sample_rate(double sample_rate)
{
    _sample_rate = sample_rate;
    _event_timer->set_sample_rate(sample_rate);
}

double RealTimeController::sample_rate() const
{
    return _sample_rate;
}

void RealTimeController::set_incoming_time(Time timestamp)
{
    _event_timer->set_incoming_time(timestamp);
}

void RealTimeController::set_outgoing_time(Time timestamp)
{
    _event_timer->set_outgoing_time(timestamp);
}

sushi::Time RealTimeController::timestamp_from_start() const
{
    uint64_t micros = _samples_since_start * 1'000'000.0 / _sample_rate;
    return std::chrono::microseconds(micros);
}

uint64_t RealTimeController::samples_since_start() const
{
    return _samples_since_start;
}

void RealTimeController::increment_samples_since_start(uint64_t amount)
{
    _samples_since_start += amount;
}

Time RealTimeController::real_time_from_sample_offset(int offset)
{
    return _event_timer->real_time_from_sample_offset(offset);
}

std::pair<bool, int> RealTimeController::sample_offset_from_realtime(Time timestamp)
{
    return _event_timer->sample_offset_from_realtime(timestamp);
}

sushi::Time RealTimeController::timestamp_from_clock()
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