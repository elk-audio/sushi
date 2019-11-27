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
 * @brief Main entry point to Sushi
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>
#include <cmath>
#include <library/rt_event.h>

#include "library/constants.h"
#include "transport.h"
#include "logging.h"

namespace sushi {
namespace engine {

void Transport::set_time(Time timestamp, int64_t samples)
{
    _time = timestamp + _latency;
    int64_t prev_samples = _sample_count;
    _update_internals();

    /* Assume that if there is a missed callback, the numbers of samples
     * will still be an even multiple of AUDIO_CHUNK_SIZE */
    auto chunks_passed = (samples - prev_samples) / AUDIO_CHUNK_SIZE;
    _sample_count = samples;

    _current_bar_beat_count +=  chunks_passed * _beats_per_chunk;
    if (_current_bar_beat_count > _beats_per_bar)
    {
        _current_bar_beat_count = std::fmod(_current_bar_beat_count, _beats_per_bar);
        _bar_start_beat_count += _beats_per_bar;
    }
    _beat_count += chunks_passed * _beats_per_chunk;
}

void Transport::set_time_signature(TimeSignature signature)
{
    assert(signature.numerator > 0);
    assert(signature.denominator > 0);
    _time_signature = signature;
}

double Transport::current_bar_beats(int samples) const
{
    double offset = _beats_per_chunk * static_cast<double>(samples) / AUDIO_CHUNK_SIZE;
    return std::fmod(_current_bar_beat_count + offset, _beats_per_bar);
}

double Transport::current_beats(int samples) const
{
    return _beat_count + _beats_per_chunk * static_cast<double>(samples) / AUDIO_CHUNK_SIZE;
}

void Transport::_update_internals()
{
    assert(_samplerate > 0.0f);
    _beats_per_chunk =  _tempo / 60.0 * static_cast<double>(AUDIO_CHUNK_SIZE) / _samplerate;
    /* Time signatures are seen in relation to 4/4 and remapped to quarter notes
     * the same way most DAWs seem to do it. This makes 3/4 and 6/8 identical and
     * they will play beatsynched with 4/4, i.e. not on triplets. */
    _beats_per_bar = 4.0f * static_cast<float>(_time_signature.numerator) /
                          static_cast<float>(_time_signature.denominator);
}

} // namespace engine
} // namespace sushi