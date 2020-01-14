#ifndef SUSHI_ENGINE_MOCKUP_H
#define SUSHI_ENGINE_MOCKUP_H

#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"

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

    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE> *in_buffer,
                       SampleBuffer<AUDIO_CHUNK_SIZE> *out_buffer,
                       ControlBuffer* /*in_controls*/ = 0,
                       ControlBuffer* /*out_controls*/ = 0) override
    {
        *out_buffer = *in_buffer;
        process_called = true;
    }

    void update_time(Time /*timestamp*/, int64_t /*samples*/) override {}

    void set_output_latency(Time /*latency*/) override {}

    float tempo() override {return _tempo;}

    void set_tempo(float tempo) override {_tempo = tempo;}

    TimeSignature time_signature() override {return _time_signature;}

    void set_time_signature(TimeSignature signature) override {_time_signature = signature;}

    PlayingMode transport_mode() override {return _playing_mode;}

    void set_transport_mode(PlayingMode mode) override {_playing_mode = mode;}

    SyncMode tempo_sync_mode() override {return _sync_mode;}

    void set_tempo_sync_mode(SyncMode mode) override {_sync_mode = mode;}

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
    float _tempo{120};
    TimeSignature _time_signature{4,4};
    PlayingMode _playing_mode{PlayingMode::STOPPED};
    SyncMode _sync_mode{SyncMode::INTERNAL};
};

#endif //SUSHI_ENGINE_MOCKUP_H
