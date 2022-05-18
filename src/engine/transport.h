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
 * @Brief Handles time, tempo and start/stop inside the engine
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_TRANSPORT_H
#define SUSHI_TRANSPORT_H

#include <memory>
#include <atomic>
#include <cassert>

#include "library/constants.h"
#include "library/event_interface.h"
#include "library/time.h"
#include "library/types.h"
#include "library/rt_event.h"
#include "library/rt_event_pipe.h"
#include "engine/base_event_dispatcher.h"

/* Forward declare Link, then we can include link.hpp from transport.cpp and
 * nowhere else, it pulls in a lot of dependencies that slow down compilation
 * significantly otherwise. */
namespace ableton {class Link;}

namespace sushi {

enum class PlayStateChange
{
    UNCHANGED,
    STARTING,
    STOPPING,
};

namespace engine {

class SushiLink;

constexpr float DEFAULT_TEMPO = 120;

class Transport
{
public:
    explicit Transport(float sample_rate, RtEventPipe* rt_event_pipe);
    ~Transport();

    /**
     * @brief Set the current time. Called from the audio thread
     * @param timestamp The current timestamp
     * @param samples   The number of samples passed
     */
    void set_time(Time timestamp, int64_t samples);

    /**
     * @brief Set the output latency, i.e. the time it takes for the audio to travel through
     *        the driver stack to a physical output, including any DAC latency. Should be
     *        called by the audio frontend
     * @param output_latency The output latency
     */
    void set_latency(Time output_latency)
    {
        _latency = output_latency;
    }

    /**
     * @brief Process a single realtime event that is to take place during the current audio
     * interrupt
     * @param event Event to process.
     */
    void process_event(const RtEvent& event);

    /**
     * @brief Set the time signature used in the engine. Called from a non-realtime thread.
     * @param signature The new time signature to use
     * @param update_via_event Set to true if the engine is running in realtime and will
     *                         pass the update as an RtEvent
     */
    void set_time_signature(TimeSignature signature, bool update_via_event);

    /**
     * @brief Set the tempo of the engine in beats (quarter notes) per minute. Called from
     *        a non-realtime thread.
     * @param tempo The new tempo in beats per minute
     * @param update_via_event Set to true if the engine is running in realtime and will
     *                         pass the update as an RtEvent
     */
    void set_tempo(float tempo, bool update_via_event);

    /**
     * @brief Return the current set playing mode
     * @return the current set playing mode
     */
    PlayingMode playing_mode() const {return _playmode;}

    /**
     * @brief Set the playing mode, i.e. playing, stopped, recording etc.. Called from
     *        a non-realtime thread.
     * @param mode The new playing mode.
     * @param update_via_event Set to true if the engine is running in realtime and will
     *                         pass the update as an RtEvent
     */
    void set_playing_mode(PlayingMode mode, bool update_via_event);

    /**
     * @brief Return the current current mode of synchronising tempo and beats
     * @return the current set mode of synchronisation
     */
    SyncMode sync_mode() const {return _syncmode;}

    /**
     * @brief Set the current mode of synchronising tempo and beats. Called from
     *        a non-realtime thread.
     * @param mode The mode of synchronisation to use
     * @param update_via_event true if the engine passes the update as an RtEvent
     */
    void set_sync_mode(SyncMode mode, bool update_via_event);

    /**
     * @return Set the sample rate.
     * @param sample_rate The current sample rate the engine is running at
     */
    void set_sample_rate(float sample_rate) {_samplerate = sample_rate;}

    /**
     * @brief Query the current time, Safe to call from rt and non-rt context. If called from
     *        an rt context it will return the time at which sample 0 of the current audio
     *        chunk being processed will appear on an output.
     * @return The current processing time.
     */
    Time current_process_time() const {return _time;}

    /**
     * @brief Query the current samplecount. Safe to call from rt and non-rt context. If called
     *        from an rt context it will return the total number of samples passed before sample
     *        0 of the current audio chunk being processed.
     * @return Total samplecount
     */
    int64_t current_samples() const {return _sample_count;}

    /**
     * @brief If the transport is currently playing or not.
     * @return true if the transport is currently playing, false if stopped
     */
    bool playing() const {return _playmode != PlayingMode::STOPPED;}

    /**
     * @brief Query the current time signature being used
     * @return A TimeSignature struct describing the current time signature
     */
    TimeSignature time_signature() const {return _time_signature;}

    /**
     * @brief Query the current tempo. Safe to call from rt and non-rt context but will
     *        only return approximate value if called from a non-rt context. If sync is
     *        not set to INTERNAL, this might be different than what was previously used
     *        as an argument to set_tempo()
     * @return A float representing the tempo in beats per minute
     */
    float current_tempo() const {return _tempo;}

    /**
    * @brief Query the position in beats (quarter notes) in the current bar with an optional
    *        sample offset from the start of the current processing chunk.
    *        The number of quarter notes in a bar will depend on the time signature set.
    *        For 4/4 time this will return a float in the range (0, 4), for 6/8 time this
    *        will return a range (0, 2). Safe to call from rt and non-rt context but will
    *        only return approximate values if called from a non-rt context
    * @param samples The number of samples offset form sample 0 in the current audio chunk
    * @return A double representing the position in the current bar.
    */
    double current_bar_beats(int samples) const;
    double current_bar_beats() const {return _current_bar_beat_count;}

    /**
     * @brief Query the current position in beats (quarter notes( with an optional sample
     *        offset from the start of the current processing chunk. This runs continuously
     *        and is monotonically increasing. Safe to call from rt and non-rt context but
     *        will only return approximate values if called from a non-rt context
     * @param samples The number of samples offset form sample 0 in the current audio chunk
     * @return A double representing the current position in quarter notes.
     */
    double current_beats(int samples) const;
    double current_beats() const {return _beat_count;}

    /**
     * @return Query the position, in beats (quarter notes), of the start of the current bar.
     *         Safe to call from rt and non-rt context but will only return approximate values
     *         if called from a non-rt context
     * @return A double representing the start position of the current bar in quarter notes
     */
    double current_bar_start_beats() const {return _bar_start_beat_count;}

    /**
     * @brief Query any playing state changes occuring during the current processing chunk.
     *        For instance, if Transport is starting, during the first chunk, Transport
     *        will report playing() as true and current_state_change() as STARTING.
     *        Subsequent chunks Transport will report playing() as true and
     *        current_state_change() as UNCHANGED.
     * @return A PlayStateChange enum with the current state change, if any.
     */
    PlayStateChange current_state_change() const {return _state_change;}

private:
    void _update_internals();
    void _update_internal_sync(int64_t samples);
    void _update_link_sync(Time timestamp);
    void _output_ppqn_ticks();
    void _set_link_playing(bool playing);
    void _set_link_tempo(float tempo);
    void _set_link_quantum(TimeSignature signature);

    int64_t         _sample_count{0};
    Time            _time{0};
    Time            _latency{0};
    double          _current_bar_beat_count{0.0};
    double          _beat_count{0.0};
    double          _bar_start_beat_count{0};
    double          _beats_per_chunk{0};
    double          _beats_per_bar{0};
    float           _samplerate{0};

    double          _last_tick_sent{0};

    float           _tempo{DEFAULT_TEMPO};
    float           _set_tempo{_tempo};
    PlayingMode     _playmode{PlayingMode::STOPPED};
    PlayingMode     _set_playmode{_playmode};
    SyncMode        _syncmode{SyncMode::INTERNAL};
    TimeSignature   _time_signature{4, 4};
    PlayStateChange _state_change{PlayStateChange::STARTING};

    RtEventPipe*    _rt_event_dispatcher;

    std::unique_ptr<SushiLink>  _link_controller;
};

} // namespace engine
} // namespace sushi

#endif //SUSHI_TRANSPORT_H
