/**
 * @Brief Interface class for objects that process audio. Processor objects can be plugins,
 *        sends, faders, mixer/channel adaptors, or chains of processors.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */
#ifndef SUSHI_PROCESSOR_H
#define SUSHI_PROCESSOR_H

#include "library/sample_buffer.h"
#include "library/plugin_events.h"

namespace sushi {

class Processor
{
public:
    virtual ~Processor() {};

    /**
     * @brief Process a single realtime event that is to take place during the next call to process
     * @param event Event to process.
     */
    virtual void process_event(BaseEvent* event) = 0;

    /**
     * @brief Process a chunk of audio.
     * @param in_buffer Input SampleBuffer
     * @param out_buffer Output SampleBuffer
     */
    virtual void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) = 0;

    // TODO - Might want to replace string ids with uuids for more consistent lookup performance
    /**
     * @brief Unique processor identifier
     * @return
     */
    virtual const std::string unique_id() { return _uuid;}


    int max_input_channels() {return _max_input_channels;}
    int max_output_channels() {return _max_output_channels;}
    int input_channels() {return  _current_input_channels;}
    int output_channels() {return _current_output_channels;}
    virtual void set_input_channels(int channels)
    {
        assert( channels <= _max_input_channels);
        _current_input_channels  = channels;
    }
    virtual void set_output_channels(int channels)
    {
        assert(channels <= _max_output_channels);
        _current_output_channels  = channels;
    }

    bool enabled() {return _enabled;}

    /* Override this for nested processors and set all sub processors to disabled */
    virtual void set_enabled(bool enabled) {_enabled = enabled;}

protected:

    std::string _uuid{""};
    /* Minimum number of output/input channels a processor should support should always be 0 */
    /* TODO - Is this a reasonable requirement? */
    int _max_input_channels{0};
    int _max_output_channels{0};

    int _current_input_channels{0};
    int _current_output_channels{0};

    bool _enabled{true};
};

/**
 * @brief Interface class for distributing events to processors.
 */
class EventCaller
{

};

} // end namespace sushi
#endif //SUSHI_PROCESSOR_H
