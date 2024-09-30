#ifndef SUSHI_LIBRARY_EVENT_DISPATCHER_ACCESSOR_H
#define SUSHI_LIBRARY_EVENT_DISPATCHER_ACCESSOR_H

#include "engine/event_dispatcher.h"
#include "test_utils/engine_mockup.h"

namespace sushi::internal::dispatcher
{

class Accessor
{
public:
    explicit Accessor(EventDispatcher& f) : _friend(f) {}

    void event_loop()
    {
        _friend._event_loop();
    }

    // Not const: it's altered in the test.
    std::atomic<bool>& running()
    {
        return _friend._running;
    }

    [[nodiscard]] bool parameter_change_queue_empty() const
    {
        return _friend._parameter_manager.parameter_change_queue_empty();
    }

    // Not const: it's altered in the test.
    EventQueue& in_queue()
    {
        return _friend._in_queue;
    }

    void crank_worker()
    {
        _friend._worker._worker();
    }

private:
    EventDispatcher& _friend;
};

class WorkerAccessor
{
public:
    explicit WorkerAccessor(Worker& f) : _friend(f) {}

    std::atomic<bool>& running()
    {
        return _friend._running;
    }

    void crank_worker()
    {
        _friend._worker();
    }

    EventQueue& queue()
    {
        return _friend._queue;
    }

private:
    Worker& _friend;
};

}

#endif //SUSHI_LIBRARY_EVENT_DISPATCHER_ACCESSOR_H
