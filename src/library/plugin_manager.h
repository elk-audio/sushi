/**
 * @Brief Internal plugin manager.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_PLUGIN_MANAGER_H
#define SUSHI_PLUGIN_MANAGER_H

#include "library/plugin_parameters.h"
#include "library/processor.h"

#include <algorithm>
#include <map>
namespace sushi {

enum class StompBoxStatus
{
    OK,
    ERROR,
    PARAMETER_ERROR,
    MEMORY_ERROR,
};

/**
 * @brief internal wrapper class for StompBox instances that keeps track
 * of all the host-related configuration.
 */

class InternalPlugin : public Processor
{
public:
    MIND_DECLARE_NON_COPYABLE(InternalPlugin)
    /**
     * @brief Create a new StompboxManager that takes ownership of instance
     * and will delete it when the manager is deleted.
     */
    InternalPlugin()
    {};

    virtual ~InternalPlugin() {};

    // Parameter registration functions

    FloatStompBoxParameter* register_float_parameter(const std::string& id,
                                                     const std::string& label,
                                                     float default_value,
                                                     FloatParameterPreProcessor* custom_pre_processor)
    {
        if (!custom_pre_processor)
        {
            custom_pre_processor = new FloatParameterPreProcessor(0.0f, 1.0f);
        }
        FloatStompBoxParameter* param = new FloatStompBoxParameter(id, label, default_value, custom_pre_processor);
        this->register_parameter(param);
        return param;
    }

    IntStompBoxParameter* register_int_parameter(const std::string& id,
                                                 const std::string& label,
                                                 int default_value,
                                                 IntParameterPreProcessor* custom_pre_processor)
    {
        if (!custom_pre_processor)
        {
            custom_pre_processor = new IntParameterPreProcessor(0, 127);
        }
        IntStompBoxParameter* param = new IntStompBoxParameter(id, label, default_value, custom_pre_processor);
        this->register_parameter(param);
        return param;
    }

    BoolStompBoxParameter* register_bool_parameter(const std::string& id,
                                                   const std::string& label,
                                                   bool default_value,
                                                   BoolParameterPreProcessor* custom_pre_processor = nullptr)
    {
        if (!custom_pre_processor)
        {
            custom_pre_processor = new BoolParameterPreProcessor(true, false);
        }
        BoolStompBoxParameter* param = new BoolStompBoxParameter(id, label, default_value, custom_pre_processor);
        this->register_parameter(param);
        return param;
    }

    StringStompBoxParameter* register_string_parameter(const std::string& id,
                                                       const std::string& label,
                                                       const std::string& default_value)
    {
        StringStompBoxParameter* param = new StringStompBoxParameter(id, label, new std::string(default_value));
        this->register_parameter(param);
        return param;
    }

    virtual DataStompBoxParameter* register_data_parameter(const std::string& id,
                                                           const std::string& label,
                                                           char* default_value)
    {
        DataStompBoxParameter* param = new DataStompBoxParameter(id, label, default_value);
        this->register_parameter(param);
        return param;
    }

    /* Inherited from Processor */
    void process_event(BaseEvent* event) override;

    // TODO: base implementation needed otherwise make_unique fails if this class is abstract.
    //       Investigate better
    virtual void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override
    {
        // Pass-through behaviour
        for (int i = 0; i < in_buffer.channel_count(); ++i)
        {
            std::copy(in_buffer.channel(i), in_buffer.channel(i) + AUDIO_CHUNK_SIZE, out_buffer.channel(i));
        }
    }


private:
    /**
    * @brief Return the parameter with the given unique id
    */
    BaseStompBoxParameter* get_parameter(const std::string& id)
    {
        auto parameter = _parameters.find(id);
        if (parameter == _parameters.end())
        {
            return nullptr;
        }
        return parameter->second.get();
    }

    void register_parameter(BaseStompBoxParameter* parameter)
    {
        _parameters.insert(std::pair<std::string, std::unique_ptr<BaseStompBoxParameter>>(parameter->id(),
                                                                  std::unique_ptr<BaseStompBoxParameter>(parameter)));
    }

    std::map<std::string, std::unique_ptr<BaseStompBoxParameter>> _parameters;
};

} // end namespace sushi
#endif //SUSHI_PLUGIN_MANAGER_H
