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
 * @brief Circular Buffer FIFO for storing events to be passed
 *        to VsT API call processEvents(VstEvents* events).
 * @warning Not thread-safe! It's ok with the current architecture where
 *          Processor::process_event(..) is called in the real-time thread
 *          before processing.
 *
 *          FIFO policy is a circular buffer which just overwrites old events,
 *          signaling the producer in case of overflow.
 *
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_VST2X_MIDI_EVENT_FIFO_H
#define SUSHI_VST2X_MIDI_EVENT_FIFO_H

#include <cmath>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"
#pragma GCC diagnostic pop

#include "library/constants.h"
#include "library/rt_event.h"
#include "library/midi_decoder.h"
#include "library/midi_encoder.h"

namespace sushi {
namespace vst2 {

template<int capacity>
class Vst2xMidiEventFIFO
{
public:
    SUSHI_DECLARE_NON_COPYABLE(Vst2xMidiEventFIFO)
    /**
     *  @brief Allocate VstMidiEvent buffer and prepare VstEvents* pointers
     */
    Vst2xMidiEventFIFO()
    {
        _midi_data = new VstMidiEvent[capacity];
        _vst_events = new VstEventsExtended();
        _vst_events->numEvents = 0;
        _vst_events->reserved = 0;

        for (int i=0; i<capacity; i++)
        {
            auto midi_ev_p = &_midi_data[i];
            midi_ev_p->type = kVstMidiType;
            midi_ev_p->byteSize = sizeof(VstMidiEvent);
            midi_ev_p->flags = kVstMidiEventIsRealtime;
            _vst_events->events[i] = reinterpret_cast<VstEvent*>(midi_ev_p);
        }
    }

    ~Vst2xMidiEventFIFO()
    {
        delete[] _midi_data;
        delete   _vst_events;
    }

    /**
     *  @brief Push an element to the FIFO.
     *  @return false if overflow happened during the operation
     */
    bool push(RtEvent event)
    {
        bool res = !_limit_reached;
        _fill_vst_event(_write_idx, event);

        _write_idx++;
        if (!_limit_reached)
        {
            _size++;
        }
        if (_write_idx == capacity)
        {
            // Reached end of buffer: wrap pointer and next call will signal overflow
            _write_idx = 0;
            _limit_reached = true;
        }

        return res;
    }

    /**
     * @brief Return pointer to stored VstEvents*
     *        You should process _all_ the returned values before any subsequent
     *        call to push(), as the internal buffer is flushed after the call.
     * @return Pointer to be passed directly to processEvents() in the real-time thread
     */
    VstEvents* flush()
    {
        _vst_events->numEvents = _size;

        // reset internal buffers
        _size = 0;
        _write_idx = 0;
        _limit_reached = false;

        return reinterpret_cast<VstEvents*>(_vst_events);
    }

private:
    /**
     * The original VstEvents struct declares events[2] to mark a variable-size
     * array. With this private struct we can avoid ugly C-style reallocations.
     */
    struct VstEventsExtended
    {
        VstInt32 numEvents;
        VstIntPtr reserved;
        VstEvent* events[capacity];
    };

    /**
    * @brief Helper to initialize VstMidiEvent inside the buffer from Event
    *
    */
    void _fill_vst_event(const int idx, RtEvent event)
    {
        auto midi_ev_p = &_midi_data[idx];
        midi_ev_p->deltaFrames = static_cast<VstInt32>(event.sample_offset());
        MidiDataByte midi_data;

        switch (event.type())
        {
            case RtEventType::NOTE_ON:
            {
                auto typed_event = event.keyboard_event();
                midi_data = midi::encode_note_on(typed_event->channel(), typed_event->note(), typed_event->velocity());
                break;
            }
            case RtEventType::NOTE_OFF:
            {
                auto typed_event = event.keyboard_event();
                midi_data = midi::encode_note_off(typed_event->channel(), typed_event->note(), typed_event->velocity());
                // For some reason, VstMidiEvent has an additional explicit field noteOffVelocity
                midi_ev_p->noteOffVelocity = midi_data[2];
                break;
            }
            case RtEventType::NOTE_AFTERTOUCH:
            {
                auto typed_event = event.keyboard_event();
                midi_data = midi::encode_poly_key_pressure(typed_event->channel(), typed_event->note(), typed_event->velocity());
                break;
            }
            case RtEventType::PITCH_BEND:
            {
                auto typed_event = event.keyboard_common_event();
                midi_data = midi::encode_pitch_bend(typed_event->channel(), typed_event->value());
                break;
            }
            case RtEventType::AFTERTOUCH:
            {
                auto typed_event = event.keyboard_common_event();
                midi_data = midi::encode_channel_pressure(typed_event->channel(), typed_event->value());
                break;
            }
            case RtEventType::MODULATION:
            {
                auto typed_event = event.keyboard_common_event();
                midi_data = midi::encode_control_change(typed_event->channel(), midi::MOD_WHEEL_CONTROLLER_NO, typed_event->value());
                break;
            }
            case RtEventType::WRAPPED_MIDI_EVENT:
            {
                auto typed_event = event.wrapped_midi_event();
                midi_data = typed_event->midi_data();
                break;
            }
            default:
                return;
        }
        std::copy(midi_data.begin(), midi_data.end(), midi_ev_p->midiData);
    }

    int _size{0};
    int _write_idx{0};
    bool _limit_reached{false};

    VstMidiEvent* _midi_data;
    VstEventsExtended* _vst_events;
};

} // namespace vst2
} // namespace sushi

#endif //SUSHI_VST2X_MIDI_EVENT_FIFO_H
