#ifndef SUSHI_ENGINE_MOCKUP_H
#define SUSHI_ENGINE_MOCKUP_H

#include "engine/engine.h"
#include "engine/event_dispatcher.h"

using namespace sushi;
using namespace sushi::engine;


// Dummy event dispatcher
class EventDispatcherMockup : public dispatcher::BaseEventDispatcher
{
public:
    ~EventDispatcherMockup()
    {
        while (!_queue.empty())
        {
            auto e = _queue.back();
            _queue.pop_back();
            delete e;
        }
    }

    int process(Event* /*event*/) override
    {
        return EventStatus::HANDLED_OK;
    }

    int poster_id() override {return EventPosterId::AUDIO_ENGINE;}

    void post_event(Event* event)
    {
        _queue.push_front(event);
    }

    bool got_event()
    {
        if (_queue.empty())
        {
            return false;
        } else
        {
            Event* e = _queue.back();
            _queue.pop_back();
            delete e;
            return true;
        }
    }

    std::unique_ptr<Event> retrieve_event()
    {
        if (_queue.empty())
        {
            return nullptr;
        } else
        {
            Event* e = _queue.back();
            _queue.pop_back();
            return std::unique_ptr<Event>(e);
        }
    }

private:
    std::deque<Event*> _queue;
};

// Bypass processor
class EngineMockup : public BaseEngine
{
public:
    EngineMockup(float sample_rate) :
        BaseEngine(sample_rate)
    {}

    ~EngineMockup()
    {}

    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE> *in_buffer ,SampleBuffer<AUDIO_CHUNK_SIZE> *out_buffer) override
    {
        *out_buffer = *in_buffer;
        process_called = true;
    }

    void update_time(Time /*timestamp*/, int64_t /*samples*/) override {}

    void set_output_latency(Time /*latency*/) override {}

    EngineReturnStatus send_rt_event(RtEvent& /*event*/) override
    {
        got_rt_event = true;
        return EngineReturnStatus::OK;
    }

    EngineReturnStatus send_async_event(RtEvent& /*event*/) override
    {
        got_event = true;
        return EngineReturnStatus::OK;
    }

    dispatcher::BaseEventDispatcher* event_dispatcher() override
    {
        return &_event_dispatcher;
    }

    bool process_called{false};
    bool got_event{false};
    bool got_rt_event{false};
private:
    EventDispatcherMockup _event_dispatcher;
};

#endif //SUSHI_ENGINE_MOCKUP_H
