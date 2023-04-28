/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Class to represent a mixer track with a chain of processors
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_TRACK_H
#define SUSHI_TRACK_H

#include <string>
#include <memory>
#include <array>
#include <vector>

#include "library/sample_buffer.h"
#include "library/internal_plugin.h"
#include "library/rt_event_fifo.h"
#include "library/constants.h"
#include "library/performance_timer.h"

#include "dsp_library/value_smoother.h"

namespace sushi {
namespace engine {

/* No real technical limit, just something arbitrarily high enough */
constexpr int MAX_TRACK_BUSES = MAX_TRACK_CHANNELS / 2;
constexpr int KEYBOARD_EVENT_QUEUE_SIZE = 256;

enum class TrackType
{
    REGULAR,
    PRE,
    POST
};

class Track : public InternalPlugin, public RtEventPipe
{
public:
    SUSHI_DECLARE_NON_COPYABLE(Track);

    /**
     * @brief Create a track
     * @param host_control Host callback object
     * @param channels The number of channels in the track
     * @param timer A timer object
     * @param pan_controls If true, create a pan control parameter on the track
     */
    Track(HostControl host_control, int channels, performance::PerformanceTimer* timer, bool pan_controls, TrackType type = TrackType::REGULAR);

    /**
     * @brief Create a track with a given number of stereo input and output buses
     *        buses are an abstraction for buses*2 channels internally.
     * @param host_control Host callback object
     * @param timer A timer object
     * @param buses The number of stereo audio buses
     */
    Track(HostControl host_control, int buses, performance::PerformanceTimer* timer);

    ~Track() override = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    /**
     * @brief Add a processor to the track's processing chain at the position before
     *        The processor with id before_position.
     *        Should be called from the audio thread or when the track is not processing.
     * @param processor A pointer to the plugin instance to add.
     * @param before_position The ObjectId of the succeeding plugin, if not set, the
     *        processor will be added to the back of the track
     * @return true if the insertion was successful, false otherwise
     */
    bool add(Processor* processor, std::optional<ObjectId> before_position = std::nullopt);

    /**
     * @brief Remove a plugin from the track.
     * @param processor The ObjectId of the processor to remove
     * @return true if the processor was found and successfully removed, false otherwise
     */
    bool remove(ObjectId processor);

    /**
     * @brief Return a SampleBuffer to an input bus
     * @param bus The index of the bus, must not be greater than the number of buses configured
     * @return A non-owning SampleBuffer pointing to the given bus
     */
    ChunkSampleBuffer input_bus(int bus)
    {
        assert(bus < _buses);
        return ChunkSampleBuffer::create_non_owning_buffer(_input_buffer, bus * 2, 2);
    }

    /**
    * @brief Return a SampleBuffer to an output bus
    * @param bus The index of the bus, must not be greater than the number of buses configured
    * @return A non-owning SampleBuffer pointing to the given bus
    */
    ChunkSampleBuffer output_bus(int bus)
    {
        assert(bus < _buses);
        return ChunkSampleBuffer::create_non_owning_buffer(_output_buffer, bus * 2, 2);
    }

    /**
     * @brief Return a SampleBuffer to an input channel
     * @param bus The index of the channel, must not be greater than the number of channels configured
     * @return A non-owning SampleBuffer pointing to the given bus
     */
    ChunkSampleBuffer input_channel(int index)
    {
        assert(index < _max_input_channels);
        return ChunkSampleBuffer::create_non_owning_buffer(_input_buffer, index, 1);
    }

    /**
    * @brief Return a SampleBuffer to an output bus
    * @param bus The index of the channel, must not be greater than the number of channels configured
    * @return A non-owning SampleBuffer pointing to the given bus
    */
    ChunkSampleBuffer output_channel(int index)
    {
        assert(index < _max_output_channels);
        return ChunkSampleBuffer::create_non_owning_buffer(_output_buffer, index, 1);
    }

    /**
     * @brief Return the number of stereo buses of the track.
     * @return The number of stereo buses on the track.
     */
    int buses() const
    {
        return _buses;
    }

    /**
     * @brief Render all processors of the track. Should be called after process_event() and
     *        after input buffers have been filled
     */
    void render();

    /**
     * @brief Static render function for passing to a thread manager
     * @param arg Void* pointing to an instance of a Track.
     */
    static void ext_render_function(void* arg)
    {
        reinterpret_cast<Track*>(arg)->render();
    }

    TrackType type() const
    {
        return _type;
    }

    /* Inherited from Processor */
    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out) override;

    void set_bypassed(bool bypassed) override;

    /* Inherited from RtEventPipe */
    void send_event(const RtEvent& event) override;

private:
    enum class PanMode
    {
        GAIN_ONLY,
        PAN_AND_GAIN,
        PAN_AND_GAIN_PER_BUS
    };

    void _common_init(PanMode mode);
    void _process_plugins(ChunkSampleBuffer& in, ChunkSampleBuffer& out);
    void _process_output_events();
    void _apply_pan_and_gain(ChunkSampleBuffer& buffer, bool muted);
    void _apply_pan_and_gain_per_bus(ChunkSampleBuffer& buffer, bool muted);
    void _apply_gain(ChunkSampleBuffer& buffer, bool muted);

    std::vector<Processor*> _processors;
    ChunkSampleBuffer _input_buffer;
    ChunkSampleBuffer _output_buffer;

    int _buses;
    PanMode _pan_mode;
    TrackType _type;

    BoolParameterValue*                               _mute_parameter;
    std::array<FloatParameterValue*, MAX_TRACK_BUSES> _gain_parameters;
    std::array<FloatParameterValue*, MAX_TRACK_BUSES> _pan_parameters;
    std::vector<std::array<ValueSmootherFilter<float>, 2>> _smoothers;

    performance::PerformanceTimer* _timer;

    RtEventFifo<KEYBOARD_EVENT_QUEUE_SIZE> _kb_event_buffer;
};

} // namespace engine
} // namespace sushi



#endif // SUSHI_TRACK_H
