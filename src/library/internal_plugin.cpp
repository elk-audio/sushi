#include "library/internal_plugin.h"

namespace sushi {

FloatParameterValue* InternalPlugin::register_float_parameter(const std::string& id,
                                                              const std::string& label,
                                                              float default_value,
                                                              float min_value,
                                                              float max_value,
                                                              FloatParameterPreProcessor* pre_proc)
{
    if (!pre_proc)
    {
        pre_proc = new FloatParameterPreProcessor(min_value, max_value);
    }
    FloatParameterDescriptor* param = new FloatParameterDescriptor(id, label, min_value, max_value, pre_proc);
    if (!this->register_parameter(param))
    {
        return nullptr;
    }
    auto value = ParameterStorage::make_float_parameter_storage(param, default_value, pre_proc);
    /* The parameter id must match the value storage index*/
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value);
    return _parameter_values.back().float_parameter_value();
}

IntParameterValue* InternalPlugin::register_int_parameter(const std::string& id,
                                                          const std::string& label,
                                                          int default_value,
                                                          int min_value,
                                                          int max_value,
                                                          IntParameterPreProcessor* pre_proc)
{
    if (! pre_proc)
    {
         pre_proc = new IntParameterPreProcessor(min_value, max_value);
    }
    IntParameterDescriptor* param = new IntParameterDescriptor(id, label, min_value, max_value, pre_proc);
    if (!this->register_parameter(param))
    {
        return nullptr;
    }
    auto value = ParameterStorage::make_int_parameter_storage(param, default_value,  pre_proc);
    /* The parameter id must match the value storage index*/
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value);
    return _parameter_values.back().int_parameter_value();
}

BoolParameterValue* InternalPlugin::register_bool_parameter(const std::string& id,
                                                            const std::string& label,
                                                            bool default_value)
{
    BoolParameterDescriptor* param = new BoolParameterDescriptor(id, label, true, false, nullptr);
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
    /* We don't provide a string value class but must push a dummy container here for ids to match */
    ParameterStorage value_storage = ParameterStorage::make_bool_parameter_storage(param, false);
    _parameter_values.push_back(value_storage);
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
    /* We don't provide a data value class but must push a dummy container here for ids to match */
    ParameterStorage value_storage = ParameterStorage::make_bool_parameter_storage(param, false);
    _parameter_values.push_back(value_storage);
    return true;
}


void InternalPlugin::process_event(RtEvent event)
{
    switch (event.type())
    {
        case RtEventType::FLOAT_PARAMETER_CHANGE:
        case RtEventType::INT_PARAMETER_CHANGE:
        case RtEventType::BOOL_PARAMETER_CHANGE:
        {
            /* These are "managed events" and not handled directly by the stompbox,
             * other events are passed on unaltered. Maybe in the future we'll do
             * some kind of conversion to StompboxEvents here to avoid exposing
             * the internal event structure to 3rd party devs. */
            auto typed_event = event.parameter_change_event();
            if (typed_event->param_id() >= _parameter_values.size())
            {
                /* Out of bounds index, this should not happen, might replace with an assert. */
                break;
            }
            auto storage = &_parameter_values[typed_event->param_id()];
            switch (storage->type())
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

void InternalPlugin::set_float_parameter_value_asynchronously(FloatParameterValue* storage, float new_value)
{
    storage->set(new_value);
    auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(), storage->value());
    output_event(e);
}

void InternalPlugin::set_int_parameter_value_asynchronously(IntParameterValue* storage, int new_value)
{
    storage->set(new_value);
    auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(), storage->value());
    output_event(e);
}

void InternalPlugin::set_bool_parameter_value_asynchronously(BoolParameterValue* storage, bool new_value)
{
    storage->set(new_value);
    auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(), storage->value());
    output_event(e);
}


} // end namespace sushi