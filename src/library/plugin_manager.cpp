#include "library/plugin_manager.h"

namespace sushi {

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
            auto parameter = get_parameter(typed_event->param_id());
            if (!parameter)
            {
                // TODO - for now, just break until we find a way of signaling an error.
                /* In fact, if we use uuids for looking up parameters then this situation
                 * should not occur, instead the error will be caught when translating from
                 * text parameter id to uuids, and that should be done in the non-rt context */
                break;
            }
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
                    static_cast<IntStompBoxParameter*>(parameter)->set(typed_event->value() > 0.5f);
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
            auto parameter = get_parameter(typed_event->param_id());
            if (parameter && parameter->type() == StompBoxParameterType::STRING)
            {
                static_cast<StringStompBoxParameter*>(parameter)->set(typed_event->value());
            }
        }
        case EventType::DATA_PARAMETER_CHANGE:
        {
            auto typed_event = static_cast<DataParameterChangeEvent*>(event);
            auto parameter = get_parameter(typed_event->param_id());
            if (parameter && parameter->type() == StompBoxParameterType::DATA)
            {
                static_cast<DataStompBoxParameter*>(parameter)->set(typed_event->value());
            }
        }

        default:
            break;
    }
}
} // end namespace sushi