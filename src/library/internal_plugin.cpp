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


StringStompBoxProperty* InternalPlugin::register_string_property(const std::string &id,
                                                                 const std::string &label,
                                                                 const std::string &default_value)
{
    StringStompBoxProperty* param = new StringStompBoxProperty(id, label, new std::string(default_value));
    bool registered = this->register_parameter(param);
    return registered? param : nullptr;
}


DataStompBoxProperty* InternalPlugin::register_data_property(const std::string &id,
                                                             const std::string &label,
                                                             BlobData default_value)
{
    DataStompBoxProperty* param = new DataStompBoxProperty(id, label, default_value);
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


void InternalPlugin::process_event(Event event)
{
    switch (event.type())
    {
        case EventType::FLOAT_PARAMETER_CHANGE:
        case EventType::INT_PARAMETER_CHANGE:
        case EventType::BOOL_PARAMETER_CHANGE:
        {
            /* These are "managed events" and not handled directly by the stompbox,
             * other events are passed on unaltered. Maybe in the future we'll do
             * some kind of conversion to StompboxEvents here to avoid exposing
             * the internal event structure to 3rd party devs. */
            auto typed_event = event.parameter_change_event();
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
            auto typed_event = event.string_parameter_change_event();
            if (typed_event->param_id() >= _parameters_by_index.size())
            {
                break;
            }
            auto parameter = _parameters_by_index[typed_event->param_id()];
            if (parameter->type() == StompBoxParameterType::STRING)
            {
                static_cast<StringStompBoxProperty*>(parameter)->set(typed_event->value());

            }
            break;
        }
        case EventType::DATA_PARAMETER_CHANGE:
        {
            auto typed_event = event.data_parameter_change_event();
            if (typed_event->param_id() >= _parameters_by_index.size())
            {
                break;
            }
            auto parameter = _parameters_by_index[typed_event->param_id()];
            if (parameter->type() == StompBoxParameterType::DATA)
            {
                static_cast<DataStompBoxProperty*>(parameter)->set(typed_event->value());
            }
            break;
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