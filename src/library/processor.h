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
#include "library/event_pipe.h"
#include "library/id_generator.h"

namespace sushi {

enum class ProcessorReturnCode
{
    OK,
    ERROR,
    PARAMETER_ERROR,
    PARAMETER_NOT_FOUND,
    MEMORY_ERROR,
};

class Processor
{
public:
    virtual ~Processor() {};

    /**
     * @brief Called by the host after instantiating the Processor, in a non-RT context. Most of the initialization, and
     * all of the initialization that can fail, should be done here. See also deinit() for deallocating
     * any resources reserved here.
     * @param sample_rate Host sample rate
     */
    virtual ProcessorReturnCode init(const int /* sample_rate */)
    {
        return ProcessorReturnCode::OK;
    }

    /**
     * @brief Process a single realtime event that is to take place during the next call to process
     * @param event Event to process.
     */
    virtual void process_event(Event event) = 0;

    /**
     * @brief Process a chunk of audio.
     * @param in_buffer Input SampleBuffer
     * @param out_buffer Output SampleBuffer
     */
    virtual void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) = 0;

    /**
     * @brief Returns a unique name for this processor
     * @return A string that uniquely identifies this processor
     */
    const std::string& name() {return _unique_name;}

    /**
     * @brief Sets the unique name of the processor.
     * @param name New Name
     */
    void set_name(const std::string& name) {_unique_name = name;}

    /**
     * @brief Returns display name for this processor
     * @return Display name as a string
     */
    const std::string& label() {return _label;}

    /**
     * @brief Sets the display name for this processor
     * @param label New display name
     */
    void set_label(const std::string& label) {_label = label;}

    /**
     * @brief Returns a unique 32 bit identifier for this processor
     * @return A unique 32 bit identifier
     */
    ObjectId id() {return _id;}

    /**
     * @brief Returns the id of a parameter with a given name
     * @param parameter_name Name of the parameter
     * @return 32 bit identifier of the parameter
     */
    virtual std::pair<ProcessorReturnCode, ObjectId> parameter_id_from_name(const std::string& /*parameter_name*/)
    {
        return std::make_pair(ProcessorReturnCode::PARAMETER_NOT_FOUND, 0u);
    }

    /**
     * @brief Set an output pipe for events.
     * @param output_pipe the output EventPipe that should receive events
     */
    virtual void set_event_output(EventPipe* pipe)
    {
        _output_pipe = pipe;
    }

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

    void output_event(Event event)
    {
        if (_output_pipe)
            _output_pipe->send_event(event);
    }

    /* Minimum number of output/input channels a processor should support should always be 0 */
    /* TODO - Is this a reasonable requirement? */
    int _max_input_channels{0};
    int _max_output_channels{0};

    int _current_input_channels{0};
    int _current_output_channels{0};

    bool _enabled{true};

private:
    /* This could easily be turned into a list if it is neccesary to broadcast events */
    EventPipe* _output_pipe{nullptr};
    /* Automatically generated unique id for identifying this processor */
    ObjectId _id{ProcessorIdGenerator::new_id()};

    std::string _unique_name{""};
    std::string _label{""};
};

/**
 * @brief Interface class for distributing events to processors.
 */
class EventCaller
{

};

} // end namespace sushi
#endif //SUSHI_PROCESSOR_H
