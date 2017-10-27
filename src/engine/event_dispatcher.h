/**
 * @Brief Main event handler in Sushi and responsible for convertion between
 *        regular and rt events.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_EVENT_DISPATCHER_H
#define SUSHI_EVENT_DISPATCHER_H

#include <deque>
#include <mutex>
#include <thread>

#include "logging.h"
#include "library/rt_event.h"
#include "library/event.h"
#include "library/event_interface.h"
#include "library/event_fifo.h"

namespace sushi {
namespace engine {class BaseEngine;}
namespace dispatcher {

constexpr int AUDIO_ENGINE_ID = 0;
constexpr char AUDIO_ENGINE_NAME[] = "Audio Engine";
constexpr std::chrono::milliseconds THREAD_PERIODICITY = std::chrono::milliseconds(1);

enum class EventDispatcherStatus
{
    OK,
    ALREADY_SUBSCRIBED,
    UNKNOWN_POSTER
};

/* Abstract base class is solely for test mockups */
class BaseEventDispatcher : public EventPoster
{
public:
    virtual ~BaseEventDispatcher()
    {}

    virtual void run() {}
    virtual void stop() {}

    virtual void post_event(Event* event) = 0;

    virtual int register_poster(EventPoster* /*poster*/) {return 0;};

    virtual EventDispatcherStatus subscribe_to_keyboard_events(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus subscribe_to_parameter_change_notifications(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}
};

class EventDispatcher : public BaseEventDispatcher
{
public:
    EventDispatcher(engine::BaseEngine* engine,
                    RtEventFifo* in_rt_queue,  RtEventFifo* out_rt_queue) : _engine{engine},
                                                                            _in_rt_queue{in_rt_queue},
                                                                            _out_rt_queue{out_rt_queue}
    {
        _posters.push_back(this);
    }

    virtual ~EventDispatcher()
    {}

    void run() override ;
    void stop() override;

    void post_event(Event* event) override;

    int register_poster(EventPoster* poster) override;

    EventDispatcherStatus subscribe_to_keyboard_events(EventPoster* receiver) override;
    EventDispatcherStatus subscribe_to_parameter_change_notifications(EventPoster* receiver) override;

    int process(Event* event) override;
    int poster_id() override {return AUDIO_ENGINE_ID;}
    const std::string& poster_name() override {return _name;}

private:

    void _event_loop();

    int _process_kb_event(KeyboardEvent* event);
    int _process_parameter_change_event(ParameterChangeEvent* event);
    int _process_string_parameter_change_event(StringPropertyChangeEvent* event);
    int _process_async_work_event(AsynchronousWorkEvent* event);

    int _process_rt_event(RtEvent& event);
    int _process_rt_keyboard_events(const KeyboardRtEvent* event);
    int _process_rt_parameter_change_events(const ParameterChangeRtEvent* event);

    void _publish_keyboard_events(Event* event);
    void _publish_parameter_events(Event* event);


    std::thread _event_thread;

    engine::BaseEngine* _engine;

    std::deque<Event*> _in_queue;
    std::deque<Event*> _out_queue;
    std::mutex _in_queue_mutex;
    std::mutex _out_queue_mutex;

    RtEventFifo* _in_rt_queue;
    RtEventFifo* _out_rt_queue;

    std::atomic<bool> _running{false};
    std::vector<EventPoster*> _posters;
    std::vector<EventPoster*> _keyboard_event_listeners;
    std::vector<EventPoster*> _parameter_change_listeners;

    std::string _name{AUDIO_ENGINE_NAME};
};

} // end namespace dispatcher
} // end namespace sushi

#endif //SUSHI_EVENT_DISPATCHER_H
