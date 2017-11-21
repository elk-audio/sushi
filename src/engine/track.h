/**
 * @Brief Class to represent a mixer track with a chain of processors
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_TRACK_H
#define SUSHI_TRACK_H

#include <memory>
#include <cassert>

#include "library/sample_buffer.h"
#include "library/internal_plugin.h"
#include "library/event_fifo.h"
#include "library/constants.h"

#include "EASTL/vector.h"

namespace sushi {
namespace engine {

/* No real technical limit, just something arbitrarily high enough */
constexpr int TRACK_MAX_CHANNELS = 8;
constexpr int TRACK_MAX_PROCESSORS = 32;


class Track : public InternalPlugin, public RtEventPipe
{
public:
    Track() : Track(TRACK_MAX_CHANNELS) {}

    Track(int channels) : _bfr_1{channels},
                          _bfr_2{channels}
    {
        _processors.reserve(TRACK_MAX_PROCESSORS);
        _max_input_channels = channels;
        _max_output_channels = channels;
        _current_input_channels = channels;
        _current_output_channels = channels;
    }

    ~Track() = default;
    MIND_DECLARE_NON_COPYABLE(Track)

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

    /* Inherited from Processor */
    void process_event(RtEvent event) override;

    void process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out) override;

    void set_bypassed(bool bypassed) override;

    void set_input_channels(int channels) override
    {
        Processor::set_input_channels(channels);
        update_channel_config();
    }

    void set_output_channels(int channels) override
    {
        Processor::set_output_channels(channels);
        update_channel_config();
    }

    /* Inherited from RtEventPipe */
    void send_event(RtEvent event) override;

private:
    /**
     * @brief Loops through the chain of processors and negotiatiates channel configuration.
     */
    void update_channel_config();

    eastl::vector<Processor*> _processors;
    ChunkSampleBuffer _bfr_1;
    ChunkSampleBuffer _bfr_2;

    // TODO - Maybe use a simpler queue here eventually
    RtEventFifo _event_buffer;
};





} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
