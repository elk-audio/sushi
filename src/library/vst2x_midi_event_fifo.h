/**
 * @Brief Circular Buffer FIFO for storing events to be passed
 *        to VsT API call processEvents(VstEvents* events).
 * @warning Not thread-safe! It's ok with the current architecture where
 *          Processor::process_event(..) is called in the real-time thread
 *          before processing.
 *
 *          FIFO policy is a circular buffer which just overwrites old events,
 *          signaling the producer in case of overflow.
 *
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_VST2X_MIDI_EVENT_FIFO_H
#define SUSHI_VST2X_MIDI_EVENT_FIFO_H

#include <cmath>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"
#pragma GCC diagnostic pop

#include "constants.h"
#include "library/rt_event.h"
#include "midi_decoder.h"

namespace sushi {
namespace vst2 {

template<int capacity>
class Vst2xMidiEventFIFO
{
public:
    MIND_DECLARE_NON_COPYABLE(Vst2xMidiEventFIFO)
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
        delete[] _vst_events;
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
    * @note TODO: Event messages do not have MIDI channel information,
    *             so at the moment just pass 0. We'll have to rewrite this
    *             when we'll have a MidiEncoder available.
    */
    void _fill_vst_event(const int idx, RtEvent event)
    {
        auto midi_ev_p = &_midi_data[idx];
        midi_ev_p->deltaFrames = static_cast<VstInt32>(event.sample_offset());
        switch (event.type())
        {
        case RtEventType::NOTE_ON:
        case RtEventType::NOTE_OFF:
        case RtEventType::NOTE_AFTERTOUCH:
        {
            auto key_event = event.keyboard_event();
            if (event.type() == RtEventType::NOTE_ON)
            {
                midi_ev_p->midiData[0] = static_cast<char>(144);
            }
            else if (event.type() == RtEventType::NOTE_OFF)
            {
                midi_ev_p->midiData[0] = static_cast<char>(128);
            }
            else if (event.type() == RtEventType::NOTE_AFTERTOUCH)
            {
                midi_ev_p->midiData[0] = static_cast<char>(160);
            }
            midi_ev_p->midiData[1] = static_cast<char>(key_event->note());
            midi_ev_p->midiData[2] = static_cast<char>(std::round(key_event->velocity() * 127));
        }
            break;

        case RtEventType::WRAPPED_MIDI_EVENT:
        {
            auto midi_event = event.wrapped_midi_event();
            auto data = midi_event->midi_data();
            for (int i=0; i<3; i++)
            {
                midi_ev_p->midiData[i] = data[i];
            }
        }
            break;

        default:
            break;
        }

        // for some reason,
        // VstMidiEvent has an additional explicit field noteOffVelocity
        if (midi::decode_message_type(reinterpret_cast<uint8_t*>(&midi_ev_p->midiData[0]), 3u)
                                       == midi::MessageType::NOTE_OFF)
        {
            midi_ev_p->noteOffVelocity = midi_ev_p->midiData[2];
        }

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
