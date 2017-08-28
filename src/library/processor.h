/**
 * @Brief Interface class for objects that process audio. Processor objects can be plugins,
 *        sends, faders, mixer/channel adaptors, or chains of processors.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */
#ifndef SUSHI_PROCESSOR_H
#define SUSHI_PROCESSOR_H

#include <map>

#include "library/sample_buffer.h"
#include "library/plugin_events.h"
#include "library/event_pipe.h"
#include "library/id_generator.h"
#include "library/plugin_parameters.h"

namespace sushi {

enum class ProcessorReturnCode
{
    OK,
    ERROR,
    PARAMETER_ERROR,
    PARAMETER_NOT_FOUND,
    MEMORY_ERROR,
    SHARED_LIBRARY_OPENING_ERROR,
    PLUGIN_ENTRY_POINT_NOT_FOUND,
    PLUGIN_LOAD_ERROR,
    PLUGIN_INIT_ERROR,
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
    virtual ProcessorReturnCode init(float /* sample_rate */)
    {
        return ProcessorReturnCode::OK;
    }

    /**
     * @brief Configure an already initialised plugin
     * @param sample_rate the new sample rate to use
     */
    virtual void configure(float /* sample_rate*/)
    {
        return;
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
     * @brief Set an output pipe for events.
     * @param output_pipe the output EventPipe that should receive events
     */
    virtual void set_event_output(EventPipe* pipe)
    {
        _output_pipe = pipe;
    }

    /**
     * @brief Get the number of parameters of this processor.
     * @return The number of registered parameters for this processor.
     */
    int parameter_count() const {return static_cast<int>(_parameters_by_index.size());};

    /**
     * @brief Get the parameter descriptor associated with a certain name
     * @param name The unique name of the parameter
     * @return A pointer to the parameter descriptor or a null pointer
     *         if there is no processor with that name
     */
    const ParameterDescriptor* parameter_from_name(const std::string& name)
    {
        auto p = _parameters.find(name);
        return (p != _parameters.end()) ? p->second.get() : nullptr;
    }

    /**
     * @brief Get the parameter descriptor associated with a certain id
     * @param id The id of the parameter
     * @return A pointer to the parameter descriptor or a null pointer
     *         if there is no processor with that id
     */
    const ParameterDescriptor* parameter_from_id(ObjectId id)
    {
        return (id < _parameters_by_index.size()) ? _parameters_by_index[id] : nullptr;
    }

    /**
     * @brief Get all controllable parameters and properties of this processor
     * @return A list of parameter objects
     */
    const std::vector<ParameterDescriptor*>& all_parameters() const
    {
        return _parameters_by_index;
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
    /**
     * @brief Set the processor to enabled or disabled. If disabled process_audio()
     *        will not be called and the processor should clear any audio tails and
     *        filter registers or anything else that could have an influence on future
     *        calls to set_enabled(true).
     * @param enabled New state of enabled.
     */
    virtual void set_enabled(bool enabled) {_enabled = enabled;}

    bool bypassed() {return _enabled;}

    /**
     * @brief Set the bypass state of the processor. If process_audio() is called
     *        in bypass mode, the processor should pass the audio unchanged while
     *        preserving any channel configuration set.
     * @param bypassed New bypass state
     */
    virtual void set_bypassed(bool bypassed) {_bypassed = bypassed;}

protected:

    /**
     * @brief Register a newly created parameter
     * @param parameter Pointer to a parameter object
     * @return true if the parameter was successfully registered, false otherwise
     */
    bool register_parameter(ParameterDescriptor* parameter)
    {
        return register_parameter(parameter, static_cast<ObjectId>(_parameters_by_index.size()));
    }

    /**
     * @brief Register a newly created parameter and manually set the id of the parameter
     * @param parameter Pointer to a parameter object
     * @param id The unique id to give to the parameter
     * @return true if the parameter was successfully registered, false otherwise
     */
    bool register_parameter(ParameterDescriptor* parameter, ObjectId id)
    {
        bool inserted = true;
        for (auto& p : _parameters_by_index)
        {
            if (p->id() == id) return false; // Don't allow duplicate parameter id:s
        }
        std::tie(std::ignore, inserted) = _parameters.insert(std::pair<std::string, std::unique_ptr<ParameterDescriptor>>(parameter->name(), std::unique_ptr<ParameterDescriptor>(parameter)));
        if (!inserted)
        {
            return false;
        }
        parameter->set_id(id);
        _parameters_by_index.push_back(parameter);
        return true;
    }

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
    bool _bypassed{false};

private:
    EventPipe* _output_pipe{nullptr};
    /* Automatically generated unique id for identifying this processor */
    ObjectId _id{ProcessorIdGenerator::new_id()};

    std::string _unique_name{""};
    std::string _label{""};

    std::map<std::string, std::unique_ptr<ParameterDescriptor>> _parameters;
    std::vector<ParameterDescriptor*> _parameters_by_index;
};


} // end namespace sushi
#endif //SUSHI_PROCESSOR_H
