#ifndef SUSHI_MOCK_EVENT_DISPATCHER_H
#define SUSHI_MOCK_EVENT_DISPATCHER_H
#include <gmock/gmock.h>

#include "engine/base_event_dispatcher.h"

using namespace sushi;
using namespace sushi::dispatcher;

class MockEventDispatcher : public dispatcher::BaseEventDispatcher
{
public:
    MockEventDispatcher() {}

    MOCK_METHOD(void,
                run,
                (),
                (override));

    MOCK_METHOD(void,
                stop,
                (),
                (override));

    MOCK_METHOD(void,
                post_event,
                (Event*),
                (override));

    MOCK_METHOD(EventDispatcherStatus,
                register_poster,
                (EventPoster*),
                (override));

    MOCK_METHOD(EventDispatcherStatus,
                subscribe_to_keyboard_events,
                (EventPoster*),
                (override));

    MOCK_METHOD(EventDispatcherStatus,
                subscribe_to_parameter_change_notifications,
                (EventPoster*),
                (override));

    MOCK_METHOD(EventDispatcherStatus,
                subscribe_to_engine_notifications,
                (EventPoster*),
                (override));

    MOCK_METHOD(EventDispatcherStatus,
                deregister_poster,
                (EventPoster*),
                (override));

    MOCK_METHOD(EventDispatcherStatus,
                unsubscribe_from_keyboard_events,
                (EventPoster*),
                (override));

    MOCK_METHOD(EventDispatcherStatus,
                unsubscribe_from_parameter_change_notifications ,
                (EventPoster*),
                (override));

    MOCK_METHOD(EventDispatcherStatus,
                unsubscribe_from_engine_notifications,
                (EventPoster*),
                (override));

    MOCK_METHOD(void,
                set_sample_rate,
                (float),
                (override));

    MOCK_METHOD(void,
                set_time,
                (Time),
                (override));

    MOCK_METHOD(int,
                process,
                (Event* event),
                (override));

    MOCK_METHOD(int,
                poster_id,
                (),
                (override));


};
#endif //SUSHI_MOCK_EVENT_DISPATCHER_H
