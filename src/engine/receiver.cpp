#include <thread>

#include "engine/receiver.h"

namespace sushi {
namespace receiver {

constexpr int MAX_RETRIES = 100;

bool AsynchronousEventReceiver::wait_for_response(EventId id, std::chrono::milliseconds timeout)
{
    int retries = 0;
    while (retries < MAX_RETRIES)
    {
        Event event;
        while (_queue->pop(event))
        {
            if (event.type() >= EventType::STOP_ENGINE)
            {
                auto typed_event = event.returnable_event();
                if (typed_event->event_id() == id)
                {
                    return typed_event->status() == ReturnableEvent::EventStatus::HANDLED_OK;
                } else
                {
                    bool status = typed_event->status() == ReturnableEvent::EventStatus::HANDLED_OK;
                    _receive_list.push_back(Node{typed_event->event_id(), status});
                }
            }
        }
        for (auto i = _receive_list.begin(); i != _receive_list.end(); ++i)
        {
            if (i->id == id)
            {
                bool status = i->status;
                _receive_list.erase(i);
                return status;
            }
        }
        std::this_thread::sleep_for(timeout / MAX_RETRIES);
        retries++;
    }
    return false;
}
} // end namespace receiver
} // end namespace sushi
