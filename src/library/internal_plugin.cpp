#include "library/internal_plugin.h"

namespace sushi {

FloatParameterValue* InternalPlugin::register_float_parameter(const std::string& id,
                                                              const std::string& label,
                                                              float default_value,
                                                              FloatParameterPreProcessor* custom_pre_processor)
{
    if (!custom_pre_processor)
    {
        custom_pre_processor = new FloatParameterPreProcessor(0.0f, 1.0f);
    }
    FloatParameterDescriptor* param = new FloatParameterDescriptor(id, label, custom_pre_processor);
    if (!this->register_parameter(param))
    {
        return nullptr;
    }
    auto value = ParameterStorage::make_float_parameter_storage(param, default_value, custom_pre_processor);
    /* The parameter id must match the value storage index*/
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value);
    return _parameter_values.back().float_parameter_value();
}

IntParameterValue* InternalPlugin::register_int_parameter(const std::string& id,
                                                          const std::string& label,
                                                          int default_value,
                                                          IntParameterPreProcessor* custom_pre_processor)
{
    if (!custom_pre_processor)
    {
        custom_pre_processor = new IntParameterPreProcessor(0, 127);
    }
    IntParameterDescriptor* param = new IntParameterDescriptor(id, label, custom_pre_processor);
    if (!this->register_parameter(param))
    {
        return nullptr;
    }
    auto value = ParameterStorage::make_int_parameter_storage(param, default_value, custom_pre_processor);
    /* The parameter id must match the value storage index*/
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value);
    return _parameter_values.back().int_parameter_value();
}

BoolParameterValue* InternalPlugin::register_bool_parameter(const std::string& id,
                                                            const std::string& label,
                                                            bool default_value,
                                                            BoolParameterPreProcessor* custom_pre_processor)
{
    if (!custom_pre_processor)
    {
        custom_pre_processor = new BoolParameterPreProcessor(true, false);
    }
    BoolParameterDescriptor* param = new BoolParameterDescriptor(id, label, custom_pre_processor);
    if (!this->register_parameter(param))
    {
        return nullptr;
    }
    ParameterStorage value_storage = ParameterStorage::make_bool_parameter_storage(param, default_value);
    /* The parameter id must match the value storage index*/
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value_storage);
    return _parameter_values.back().bool_parameter_value();
}


bool InternalPlugin::register_string_property(const std::string &id,
                                              const std::string &label)
{
    StringPropertyDescriptor* param = new StringPropertyDescriptor(id, label);
    if (!this->register_parameter(param))
    {
        return false;
    }
    return true;
}


bool InternalPlugin::register_data_property(const std::string &id,
                                            const std::string &label)
{
    DataPropertyDescriptor* param = new DataPropertyDescriptor(id, label);
    if (!this->register_parameter(param))
    {
        return false;
    }
    return true;
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
            auto storage = &_parameter_values[typed_event->param_id()];
            switch (parameter->type())
            {
                case ParameterType::FLOAT:
                {
                    storage->float_parameter_value()->set(typed_event->value());
                    break;
                }
                case ParameterType::INT:
                {
                    storage->int_parameter_value()->set(typed_event->value());
                    break;
                }
                case ParameterType::BOOL:
                {
                    storage->bool_parameter_value()->set_values(typed_event->value(), typed_event->value());
                    break;
                }
                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
}


} // end namespace sushi