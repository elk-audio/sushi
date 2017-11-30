/**
 * @Brief Class to represent a mixer track with a chain of processors
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_TRACK_H
#define SUSHI_TRACK_H

#include <memory>
#include <cassert>
#include <array>
#include <vector>

#include "library/sample_buffer.h"
#include "library/internal_plugin.h"
#include "library/event_fifo.h"
#include "library/constants.h"


namespace sushi {
namespace engine {

/* No real technical limit, just something arbitrarily high enough */
constexpr int TRACK_MAX_CHANNELS = 8;
constexpr int TRACK_MAX_BUSSES = TRACK_MAX_CHANNELS / 4;
constexpr int TRACK_MAX_PROCESSORS = 32;
constexpr float PAN_GAIN_3_DB = 1.412537f;

class Track : public InternalPlugin, public RtEventPipe
{
public:
    MIND_DECLARE_NON_COPYABLE(Track);

    /**
     * @brief Create a track with a given number of channels
     * @param channels The number of channels in the track.
     *                 Note that even mono tracks have a stereo output bus
     */
    Track(int channels);

    /**
     * @brief Create a track with a given number of stereo input and output busses
     *        Busses are an abstraction for busses*2 channels internally.
     * @param input_buffers The number of input busses
     * @param output_buffers The number of output busses
     */
    Track(int input_busses, int output_busses);

    ~Track() = default;

    /**
     * @brief Adds a plugin to the end of the track.
     * @param The plugin to add.
     */
    bool add(Processor* processor);

    /**
     * @brief Remove a plugin from the track.
     * @param processor The ObjectId of the processor to remove
     * @return true if the processor was found and succesfully removed, false otherwise
     */
    bool remove(ObjectId processor);

    /**
     * @brief Return a reference to the track's input buffer.
     * @param buffer Audio data from this buffer will be sent to the track.
     */
    ChunkSampleBuffer& input_buffer()
    {
        return _input_buffer;
    }

    /**
     * @brief Return a reference to the track's output buffer. After a call to render(),
     *        this contains the tracks output, panned and ready.
     * @return A reference to an audio buffer that contains the output of the track.
     */
    const ChunkSampleBuffer& output_buffer()
    {
        return _output_buffer;
    }

    /**
     * @brief Return a SampleBuffer to an input bus
     * @param bus The index of the bus, must not be greater than the number of busses configured
     * @return A non-owning SampleBuffer pointing to the given bus
     */
    ChunkSampleBuffer input_bus(int bus)
    {
        assert(bus < _input_busses);
        return ChunkSampleBuffer::create_non_owning_buffer(_input_buffer, bus * 2, 2);
    }

    /**
    * @brief Return a SampleBuffer to an output bus
    * @param bus The index of the bus, must not be greater than the number of busses configured
    * @return A non-owning SampleBuffer pointing to the given bus
    */
    ChunkSampleBuffer output_bus(int bus)
    {
        assert(bus < _output_busses);
        return ChunkSampleBuffer::create_non_owning_buffer(_output_buffer, bus * 2, 2);
    }

    /**
     * @brief Return the number of input busses of the track.
     * @return The number of input busses on the track.
     */
    int input_busses()
    {
        return _input_busses;
    }

    /**
     * @brief Return the number of input busses of the track.
     * @return The number of input busses on the track.
     */
    int output_busses()
    {
        return _output_busses;
    }

    /**
     * @brief Render all processors of the track. Should be called after events have been
     */
    void render();

    /* Inherited from Processor */
    void process_event(RtEvent event) override;

    void process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out) override;

    void set_bypassed(bool bypassed) override;

    void set_input_channels(int channels) override
    {
        Processor::set_input_channels(channels);
        _update_channel_config();
    }

    void set_output_channels(int channels) override
    {
        Processor::set_output_channels(channels);
        _update_channel_config();
    }

    /* Inherited from RtEventPipe */
    void send_event(RtEvent event) override;

private:
    void _init_parameters();
    void _update_channel_config();
    void _process_output_events();

    std::vector<Processor*> _processors;
    ChunkSampleBuffer _input_buffer;
    ChunkSampleBuffer _output_buffer;

    // TODO - Maybe use a simpler queue here eventually
    RtEventFifo _event_buffer;

    int _input_busses;
    int _output_busses;
    bool _multibus;


    std::array<FloatParameterValue*, TRACK_MAX_BUSSES> _gain_parameters;
    std::array<FloatParameterValue*, TRACK_MAX_BUSSES> _pan_parameters;
};

void apply_pan_and_gain(ChunkSampleBuffer& buffer, float gain, float pan);


} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
