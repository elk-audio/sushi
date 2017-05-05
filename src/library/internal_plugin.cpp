#include "library/internal_plugin.h"

namespace sushi {

FloatStompBoxParameter* InternalPlugin::register_float_parameter(const std::string& id,
                                                 const std::string& label,
                                                 float default_value,
                                                 FloatParameterPreProcessor* custom_pre_processor)
{
    if (!custom_pre_processor)
    {
        custom_pre_processor = new FloatParameterPreProcessor(0.0f, 1.0f);
    }
    FloatStompBoxParameter* param = new FloatStompBoxParameter(id, label, default_value, custom_pre_processor);
    bool registered = this->register_parameter(param);
    return registered? param : nullptr;
}

IntStompBoxParameter* InternalPlugin::register_int_parameter(const std::string& id,
                                             const std::string& label,
                                             int default_value,
                                             IntParameterPreProcessor* custom_pre_processor)
{
    if (!custom_pre_processor)
    {
        custom_pre_processor = new IntParameterPreProcessor(0, 127);
    }
    IntStompBoxParameter* param = new IntStompBoxParameter(id, label, default_value, custom_pre_processor);
    bool registered = this->register_parameter(param);
    return registered? param : nullptr;
}

BoolStompBoxParameter* InternalPlugin::register_bool_parameter(const std::string& id,
                                               const std::string& label,
                                               bool default_value,
                                               BoolParameterPreProcessor* custom_pre_processor)
{
    if (!custom_pre_processor)
    {
        custom_pre_processor = new BoolParameterPreProcessor(true, false);
    }
    BoolStompBoxParameter* param = new BoolStompBoxParameter(id, label, default_value, custom_pre_processor);
    bool registered = this->register_parameter(param);
    return registered? param : nullptr;
}


StringStompBoxParameter* InternalPlugin::register_string_parameter(const std::string& id,
                                                   const std::string& label,
                                                   const std::string& default_value)
{
    StringStompBoxParameter* param = new StringStompBoxParameter(id, label, new std::string(default_value));
    bool registered = this->register_parameter(param);
    return registered? param : nullptr;
}


DataStompBoxParameter* InternalPlugin::register_data_parameter(const std::string& id,
                                                       const std::string& label,
                                                       char* default_value)
{
    DataStompBoxParameter* param = new DataStompBoxParameter(id, label, default_value);
    bool registered = this->register_parameter(param);
    return registered? param : nullptr;
}


std::pair<ProcessorReturnCode, ObjectId> InternalPlugin::parameter_id_from_name(const std::string& parameter_name)
{
    auto parameter = get_parameter(parameter_name);
    if (parameter)
    {
        return std::make_pair(ProcessorReturnCode::OK, parameter->id());
    }
    return std::make_pair(ProcessorReturnCode::PARAMETER_NOT_FOUND, 0u);
}


void InternalPlugin::process_event(BaseEvent* event)
{
    assert(event);
    switch (event->type())
    {
        case EventType::FLOAT_PARAMETER_CHANGE:
        case EventType::INT_PARAMETER_CHANGE:
        case EventType::BOOL_PARAMETER_CHANGE:
        {
            /* These are "managed events" and not handled directly by the stompbox,
             * other events are passed on unaltered. Maybe in the future we'll do
             * some kind of conversion to StompboxEvents here to avoid exposing
             * the internal event structure to 3rd party devs. */
            auto typed_event = static_cast<ParameterChangeEvent*>(event);
            if (typed_event->param_id() >= _parameters_by_index.size())
            {
                /* Out of bounds index, this should not happen, might replace with an assert. */
                break;
            }
            auto parameter = _parameters_by_index[typed_event->param_id()];
            switch (parameter->type())
            {
                case StompBoxParameterType::FLOAT:
                {
                    static_cast<FloatStompBoxParameter*>(parameter)->set(typed_event->value());
                    break;
                }
                case StompBoxParameterType::INT:
                {
                    static_cast<IntStompBoxParameter*>(parameter)->set(static_cast<int>(typed_event->value()));
                    break;
                }
                case StompBoxParameterType::BOOL:
                {
                    static_cast<BoolStompBoxParameter*>(parameter)->set(typed_event->value() > 0.5f? true : false);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case EventType::STRING_PARAMETER_CHANGE:
        {
            auto typed_event = static_cast<StringParameterChangeEvent*>(event);
            if (typed_event->param_id() >= _parameters_by_index.size())
            {
                break;
            }
            auto parameter = _parameters_by_index[typed_event->param_id()];
            if (parameter->type() == StompBoxParameterType::STRING)
            {
                static_cast<StringStompBoxParameter*>(parameter)->set(typed_event->value());
            }
        }
        case EventType::DATA_PARAMETER_CHANGE:
        {
            auto typed_event = static_cast<DataParameterChangeEvent*>(event);
            if (typed_event->param_id() >= _parameters_by_index.size())
            {
                break;
            }
            auto parameter = _parameters_by_index[typed_event->param_id()];
            if (parameter->type() == StompBoxParameterType::DATA)
            {
                static_cast<DataStompBoxParameter*>(parameter)->set(typed_event->value());
            }
        }

        default:
            break;
    }
}

bool InternalPlugin::register_parameter(BaseStompBoxParameter* parameter)
{
    parameter->set_id(static_cast<ObjectId>(_parameters_by_index.size()));
    _parameters_by_index.push_back(parameter);
    bool inserted = true;
    std::tie(std::ignore, inserted) = _parameters.insert(std::pair<std::string, std::unique_ptr<BaseStompBoxParameter>>(parameter->name(),
                                                                                std::unique_ptr<BaseStompBoxParameter>(parameter)));
    return inserted;
}


} // end namespace sushi