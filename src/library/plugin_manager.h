/**
 * @Brief Internal plugin manager.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_PLUGIN_MANAGER_H
#define SUSHI_PLUGIN_MANAGER_H

#include "library/plugin_parameters.h"
#include "plugin_interface.h"

#include <map>
namespace sushi {

/**
 * @brief internal wrapper class for StompBox instances that keeps track
 * of all the host-related configuration.
 */

class StompBoxManager : public StompBoxController
{
public:
    MIND_DECLARE_NON_COPYABLE(StompBoxManager)
    /**
     * @brief Create a new StompboxManager that takes ownership of instance
     * and will delete it when the manager is deleted.
     */
    StompBoxManager(StompBox* instance) : _instance(instance) {}

    virtual ~StompBoxManager() {};

    /**
     * @brief Return a pointer to the stompbox instance.
     */
    StompBox* instance() {return _instance.get();}

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

    // Parameter registration functions inherited from StompBoxController

    FloatStompBoxParameter* register_float_parameter(const std::string& id,
                                                     const std::string& label,
                                                     float default_value,
                                                     FloatParameterPreProcessor* custom_pre_processor) override
    {
        if (!custom_pre_processor)
        {
            custom_pre_processor = new FloatParameterPreProcessor(0.0f, 1.0f);
        }
        FloatStompBoxParameter* param = new FloatStompBoxParameter(id, label, default_value, custom_pre_processor);
        this->register_parameter(param);
        return param;
    };

    IntStompBoxParameter* register_int_parameter(const std::string& id,
                                                 const std::string& label,
                                                 int default_value,
                                                 IntParameterPreProcessor* custom_pre_processor) override
    {
        if (!custom_pre_processor)
        {
            custom_pre_processor = new IntParameterPreProcessor(0, 127);
        }
        IntStompBoxParameter* param = new IntStompBoxParameter(id, label, default_value, custom_pre_processor);
        this->register_parameter(param);
        return param;
    };

    BoolStompBoxParameter* register_bool_parameter(const std::string& id,
                                                   const std::string& label,
                                                   bool default_value,
                                                   BoolParameterPreProcessor* custom_pre_processor = nullptr) override
    {
        if (!custom_pre_processor)
        {
            custom_pre_processor = new BoolParameterPreProcessor(true, false);
        }
        BoolStompBoxParameter* param = new BoolStompBoxParameter(id, label, default_value, custom_pre_processor);
        this->register_parameter(param);
        return param;
    };


private:
    void register_parameter(BaseStompBoxParameter* parameter)
    {
        _parameters.insert(std::pair<std::string, std::unique_ptr<BaseStompBoxParameter>>(parameter->id(),
                                                                  std::unique_ptr<BaseStompBoxParameter>(parameter)));
    }

    std::unique_ptr<StompBox> _instance;
    std::map<std::string, std::unique_ptr<BaseStompBoxParameter>> _parameters;
};

} // end namespace sushi
#endif //SUSHI_PLUGIN_MANAGER_H
