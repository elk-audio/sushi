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
#include <vector>

#include "library/sample_buffer.h"
#include "library/rt_event.h"
#include "library/rt_event_pipe.h"
#include "library/id_generator.h"
#include "library/plugin_parameters.h"
#include "engine/host_control.h"

namespace sushi {

enum class ProcessorReturnCode
{
    OK,
    ERROR,
    PARAMETER_ERROR,
    PARAMETER_NOT_FOUND,
    MEMORY_ERROR,
    UNSUPPORTED_OPERATION,
    SHARED_LIBRARY_OPENING_ERROR,
    PLUGIN_ENTRY_POINT_NOT_FOUND,
    PLUGIN_LOAD_ERROR,
    PLUGIN_INIT_ERROR,
};

class Processor
{
public:
    explicit Processor(HostControl host_control) : _host_control(host_control) {}

    virtual ~Processor() = default;

    /**
     * @brief Called by the host after instantiating the Processor, in a non-RT context. Most of the initialization, and
     * all of the initialization that can fail, should be done here.
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
    virtual void process_event(RtEvent event) = 0;

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
    const std::string& name() const {return _unique_name;}

    /**
     * @brief Sets the unique name of the processor.
     * @param name New Name
     */
    void set_name(const std::string& name) {_unique_name = name;}

    /**
     * @brief Returns display name for this processor
     * @return Display name as a string
     */
    const std::string& label() const {return _label;}

    /**
     * @brief Sets the display name for this processor
     * @param label New display name
     */
    void set_label(const std::string& label) {_label = label;}

    /**
     * @brief Returns a unique 32 bit identifier for this processor
     * @return A unique 32 bit identifier
     */
    ObjectId id() const {return _id;}

    /**
     * @brief Set an output pipe for events.
     * @param output_pipe the output EventPipe that should receive events
     */
    virtual void set_event_output(RtEventPipe* pipe)
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
    const ParameterDescriptor* parameter_from_name(const std::string& name) const
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
    const ParameterDescriptor* parameter_from_id(ObjectId id) const
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

    /**
     * @brief Set the number of input audio channels of the Processor.
     *        Must not be set to more channels than what is reported by
     *        max_input_channels()
     * @param channels The new number of input channels
     */
    virtual void set_input_channels(int channels)
    {
        assert(channels <= _max_input_channels);
        _current_input_channels = channels;
    }

    /**
     * @brief Set the number of output audio channels of the Processor.
     *        Must not be set to more channels than what is reported by
     *        max_output_channels()
     * @param channels The new number of output channels
     */
    virtual void set_output_channels(int channels)
    {
        assert(channels <= _max_output_channels);
        _current_output_channels = channels;
    }

    virtual bool enabled() {return _enabled;}

    /* Override this for nested processors and set all sub processors to disabled */
    /**
     * @brief Set the processor to enabled or disabled. If disabled process_audio()
     *        will not be called and the processor should clear any audio tails and
     *        filter registers or anything else that could have an influence on future
     *        calls to set_enabled(true).
     * @param enabled New state of enabled.
     */
    virtual void set_enabled(bool enabled) {_enabled = enabled;}

    virtual bool bypassed() const {return _bypassed;}

    /**
     * @brief Set the bypass state of the processor. If process_audio() is called
     *        in bypass mode, the processor should pass the audio unchanged while
     *        preserving any channel configuration set.
     * @param bypassed New bypass state
     */
    virtual void set_bypassed(bool bypassed) {_bypassed = bypassed;}

    /**
     * @brief Get the value of the  parameter with parameter_id, safe to call from
     *        a non rt-thread
     * @param parameter_id The Id of the requested parameter
     * @return The current value of the parameter if the return code is OK
     */
    virtual std::pair<ProcessorReturnCode, float> parameter_value(ObjectId /*parameter_id*/) const
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0.0f};
    };

    /**
     * @brief Get the value of the  parameter with parameter_id, safe to call from
     *        a non rt-thread
     * @param parameter_id The Id of the requested parameter
     * @return The current value normalised to a 0 to 1 range, if the return code is OK
     */
    virtual std::pair<ProcessorReturnCode, float> parameter_value_normalised(ObjectId /*parameter_id*/) const
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0.0f};
    };

    /**
     * @brief Get the value of the parameter with parameter_id formatted as a string,
     *        safe to call from a non rt-thread
     * @param parameter_id The Id of the requested parameter
     * @return The current value formatted as a string, if the return code is OK
     */
    virtual std::pair<ProcessorReturnCode, std::string> parameter_value_formatted(ObjectId /*parameter_id*/) const
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, ""};
    };

    /**
     * @brief Whether or not the processor supports programs/presets
     * @return True if the processor supports programs, false otherwise
     */
    virtual bool supports_programs() const {return false;}

    /**
     * @brief Get the number of stored programs in the processor
     * @return The number of programs
     */
    virtual int program_count() const {return 0;}

    /**
     * @brief Get the currently active program
     * @return The index of the current program
     */
    virtual int current_program() const {return 0;}

    /**
     * @brief Get the name of the currently active program
     * @return The name of the current program
     */
    virtual std::string current_program_name() const {return "";}

    /**
     * @brief Get the name of a stored program
     * @param program The index (starting from 0) of the requested program
     * @return The name of the string if return is OK, otherwise the first member of the
     *         pair contains an error code
     */
    virtual std::pair<ProcessorReturnCode, std::string> program_name(int /*program*/) const
    {
        return {ProcessorReturnCode::UNSUPPORTED_OPERATION, ""};
    }

    /**
     * @brief Get all stored programs
     * @return An std::vector with all program names and where the index in the vector
     *         represents the program index, if the return is OK, otherwise the first
     *         member of the pair contains an error code
     */
    virtual std::pair<ProcessorReturnCode, std::vector<std::string>> all_program_names() const
    {
        return {ProcessorReturnCode::UNSUPPORTED_OPERATION, std::vector<std::string>()};
    }

    /**
     * @brief Set a new program to the processor. Called from a non-rt thread
     * @param program The id of the new program to use
     * @return OK if the operation was successful, error code otherwise
     */
    virtual ProcessorReturnCode set_program(int /*program*/) {return ProcessorReturnCode::UNSUPPORTED_OPERATION;}

    /**
     * @brief Connect a cv input to a parameter of the processor
     * @param parameter_id The id of the parameter to connect to
     * @param cv_input_id The id of the cv input
     * @return ProcessorReturnCode::OK on success, error code on failure
     */
     // TODO - Handled by the engine for now
    virtual ProcessorReturnCode connect_cv_to_parameter(ObjectId parameter_id, int cv_input_id);

    /**
     * @brief Connect a parameter of the processor to a cv out so that rt updates of
     *        the parameter will be sent to the cv output
     * @param parameter_id The id of the parameter to connect from
     * @param cv_input_id The id of the cv output
     * @return ProcessorReturnCode::OK on success, error code on failure
     */
    virtual ProcessorReturnCode connect_cv_from_parameter(ObjectId parameter_id, int cv_output_id);
    /**
     * @brief Connect note on and off events with a particular channel and note number
     *        from this processor to a gate output.
     * @param gate_output_id The gate output to output to.
     * @param channel Only events with this channel will be routed.
     * @param note_no The note number that will be used for this gate output.
     * @return ProcessorReturnCode::OK on success, error code on failure
     */
    virtual ProcessorReturnCode connect_gate_from_processor(int gate_output_id, int channel, int note_no);

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
    bool register_parameter(ParameterDescriptor* parameter, ObjectId id);

    void output_event(RtEvent event)
    {
        if (_output_pipe)
            _output_pipe->send_event(event);
    }

    /**
     * @brief Handle parameter updates if connected to cv outputs and send cv output event if
     *        the parameter is connected to a cv output
     * @param parameter_id The id of the parameter
     * @param value The new value of the parameter change
     * @return true If there is an active outgoing connection from this parameter, false otherwise
     */
    bool maybe_output_cv_value(ObjectId parameter_id, float value);

    /**
    * @brief Utility function do to general bypass/passthrough audio processing.
    *        Useful for processors that don't implement this on their own.
    * @param in_buffer Input SampleBuffer
    * @param out_buffer Output SampleBuffer
    */
    void bypass_process(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer);

    /* Minimum number of output/input channels a processor should support should always be 0 */
    /* TODO - Is this a reasonable requirement? */
    int _max_input_channels{0};
    int _max_output_channels{0};

    int _current_input_channels{0};
    int _current_output_channels{0};

    bool _enabled{false};
    bool _bypassed{false};

    HostControl _host_control;

    struct CvInConnection
    {
        ObjectId parameter_id;
        bool enabled;
    };
    struct CvOutConnection
    {
        ObjectId parameter_id;
        int cv_id;
    };

    std::array<CvInConnection, MAX_ENGINE_CV_IO_PORTS> _cv_in_connections;
    std::array<CvOutConnection, MAX_ENGINE_CV_IO_PORTS> _cv_out_connections;
    int _outgoing_cv_connections{0};

private:
    RtEventPipe* _output_pipe{nullptr};
    /* Automatically generated unique id for identifying this processor */
    ObjectId _id{ProcessorIdGenerator::new_id()};

    std::string _unique_name{""};
    std::string _label{""};

    std::map<std::string, std::unique_ptr<ParameterDescriptor>> _parameters;
    std::vector<ParameterDescriptor*> _parameters_by_index;
};

constexpr std::chrono::duration<float, std::ratio<1,1>> BYPASS_RAMP_TIME = std::chrono::milliseconds(10);

static int chunks_to_ramp(float sample_rate)
{
    return static_cast<int>(std::floor(std::max(1.0f, (sample_rate * BYPASS_RAMP_TIME.count() / AUDIO_CHUNK_SIZE))));
}

/**
 * @brief Convience class to encapsulate the bypass state logic and when to ramp audio up
 *        or down in order to avoid audio clicks and artefacts.
 */
class BypassManager
{
public:
    MIND_DECLARE_NON_COPYABLE(BypassManager);

    BypassManager() = default;
    explicit BypassManager(bool bypassed_by_default) :
            _state(bypassed_by_default? BypassState::BYPASSED : BypassState::NOT_BYPASSED) {}

    /**
     * @brief Check whether or not bypass is enabled
     * @return true if bypass is enabled
     */
    bool bypassed() const {return _state == BypassState::BYPASSED || _state == BypassState::RAMPING_DOWN;};

    /**
     * @brief Set the bypass state
     * @param bypass_enabled If true sets bypass enabled, if false turns off bypass
     * @param sample_rate The current sample rate, used for calculating ramp time
     */
    void set_bypass(bool bypass_enabled, float sample_rate)
    {
        if (bypass_enabled && this->bypassed() == false)
        {
            _state = BypassState::RAMPING_DOWN;
            _ramp_chunks = chunks_to_ramp(sample_rate);
            _ramp_count = _ramp_chunks;
        }
        if (bypass_enabled == false && this->bypassed())
        {
            _state = BypassState::RAMPING_UP;
            _ramp_chunks = chunks_to_ramp(sample_rate);
            _ramp_count = 0;
        }
    }

    /**
     * @return true if the processors processing functions needs to be called, false otherwise
     */
    bool should_process() const {return _state != BypassState::BYPASSED;}

    /**
     * @return true if the processor output should be ramped, false if it doesn't need volume ramping
     */
    bool should_ramp() const {return _state == BypassState::RAMPING_DOWN || _state == BypassState::RAMPING_UP;};

    /**
     * @brief Does volume ramping on the buffer passed to the function based on the current bypass state
     * @param output_buffer The buffer to apply the volume ramp
     */
    void ramp_output(ChunkSampleBuffer& output_buffer)
    {
        auto [start, end] = get_ramp();
        output_buffer.ramp(start, end);
    }

    /**
     * @brief Does crossfade volume ramping between input and output buffer based in the
     *        current bypass state.
     * @param input_buffer The input buffer to the processor, what will be crossfaded to if
     *                     bypass is enabled
     * @param output_buffer A filled output buffer from the processor, what will be crossfaded
     *                      to if bypass is disabled
     * @param input_channels The current number of input channels of the processor
     * @param output_channels The current number of output channels of the processor
     */
    void crossfade_output(const ChunkSampleBuffer& input_buffer, ChunkSampleBuffer& output_buffer,
                          int input_channels, int output_channels);

private:
    enum class BypassState
    {
        NOT_BYPASSED,
        BYPASSED,
        RAMPING_DOWN,
        RAMPING_UP
    };

    std::pair<float, float> get_ramp();

    BypassState _state{BypassState::NOT_BYPASSED};
    int _ramp_chunks{0};
    int _ramp_count{0};
};

}; // end namespace sushi
#endif //SUSHI_PROCESSOR_H
