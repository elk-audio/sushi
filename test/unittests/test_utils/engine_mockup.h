#ifndef SUSHI_ENGINE_MOCKUP_H
#define SUSHI_ENGINE_MOCKUP_H

#include <memory>

#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"
#include "dummy_processor.h"
#include "engine/midi_dispatcher.h"

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::midi;
using namespace sushi::internal::engine;
using namespace sushi::internal::midi_dispatcher;

// Dummy event dispatcher
class EventDispatcherMockup : public dispatcher::BaseEventDispatcher
{
public:
    enum class Action {
        Discard,
        Execute
    };

    ~EventDispatcherMockup() override
    {
        while (!_queue.empty())
        {
            // unique_ptr's will delete content as they go out of scope.
            _queue.pop_back();
        }
    }

    void run() override {}
    void stop() override {}

    void set_sample_rate(float /*sample_rate*/) override {}
    void set_time(Time /*timestamp*/) override {}

    int dispatch(std::unique_ptr<Event>&& /*event*/) override
    {
        return EventStatus::HANDLED_OK;
    }

    void post_event(std::unique_ptr<Event>&& event) override
    {
        _queue.push_front(std::move(event));
    }

    /**
     * Call this to check if an event was received, and then discard it.
     * @return
     */
    bool got_event()
    {
        if (_queue.empty())
        {
            return false;
        }
        else
        {
            // unique_ptr's will delete content as they go out of scope.
            _queue.pop_back();
            return true;
        }
    }

    /**
     * Call this to check if an engine event was received, execute it, and then discard it.
     * Non-engine events are discarded, and only the first engine event found is executed.
     * @param engine reference for the events execute method.
     * @return The execution status of the event.
     */
    int execute_engine_event(BaseEngine* engine)
    {
        EventStatus::EventStatus status = EventStatus::NOT_HANDLED;

        while (!_queue.empty())
        {
            auto event = _queue.back().get();

            // There can be notification events before, which we want to ignore when mocking.
            if (event->is_engine_event())
            {
                /* TODO: If we go with Lambdas in all executable events,
                 *  the engine can just be captured in the Lambda.
                 *  If not, it should be a parameter to the Event class constructor.
                 */
                auto typed_event = static_cast<EngineEvent*>(event);

                status = static_cast<EventStatus::EventStatus>(typed_event->execute(engine));

                _queue.pop_back();

                return status;
            }
            else
            {
                _queue.pop_back();
            }
        }

        return status;
    }

    std::unique_ptr<Event> retrieve_event()
    {
        if (_queue.empty())
        {
            return nullptr;
        }
        else
        {
            auto e = std::move(_queue.back());
            _queue.pop_back();
            return e;
        }
    }

private:
    std::deque<std::unique_ptr<Event>> _queue;
};

class ProcessorContainerMockup : public BaseProcessorContainer
{
public:
    ProcessorContainerMockup() : _processor(std::make_shared<DummyProcessor>(HostControl(nullptr, nullptr, nullptr))),
                                 _track(std::make_shared<Track>(HostControl(nullptr, nullptr, nullptr), 2, nullptr)) {}

    bool add_processor(std::shared_ptr<Processor> /*processor*/) override {return true;}

    bool add_track(std::shared_ptr<Track> /*track*/) override {return true;}

    bool remove_processor(ObjectId /*id*/) override {return true;}

    bool remove_track(ObjectId /*track_id*/) override {return true;}

    bool add_to_track(std::shared_ptr<Processor> /*processor*/, ObjectId /*track_id*/, std::optional<ObjectId> /*before_id*/) override {return true;}

    bool remove_from_track(ObjectId /*processor_id*/, ObjectId /*track_id*/) override {return true;}

    bool processor_exists(ObjectId /*id*/) const override {return true;}

    bool processor_exists(const std::string& /*name*/) const override {return true;}

    std::vector<std::shared_ptr<const Processor>> all_processors() const override {return {_processor};}

    std::shared_ptr<Processor> mutable_processor(ObjectId /*id*/) const override {return _processor;}

    std::shared_ptr<Processor> mutable_processor(const std::string& /*name*/) const override {return _processor;}

    std::shared_ptr<const Processor> processor(ObjectId /*id*/) const override {return _processor;}

    std::shared_ptr<const Processor> processor(const std::string& /*name*/) const override {return _processor;}

    std::shared_ptr<Track> mutable_track(ObjectId /*track_id*/) const override {return _track;}

    std::shared_ptr<Track> mutable_track(const std::string& /*track_name*/) const override {return _track;}

    std::shared_ptr<const Track> track(ObjectId /*track_id*/) const override {return _track;}

    std::shared_ptr<const Track> track(const std::string& /*name*/) const override {return _track;}

    std::vector<std::shared_ptr<const Processor>> processors_on_track(ObjectId /*track_id*/) const override {return {_processor};}

    std::vector<std::shared_ptr<const Track>> all_tracks() const override {return {_track};}

private:
    std::shared_ptr<Processor> _processor;
    std::shared_ptr<Track>     _track;
};

class EngineMockup : public BaseEngine
{
public:
    EngineMockup(float sample_rate) : BaseEngine(sample_rate),
                                      _transport(sample_rate, &_rt_event_output)
    {}

    ~EngineMockup() override
    {}

    void
    process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer,
                  SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer,
                  ControlBuffer*, ControlBuffer*, Time, int64_t) override
    {
        *out_buffer = *in_buffer;
        process_called = true;
    }

    void set_output_latency(Time /*latency*/) override {}

    void set_tempo(float /*tempo*/) override {}

    void set_time_signature(TimeSignature /*signature*/) override {}

    void set_transport_mode(PlayingMode /*mode*/) override {}

    void set_tempo_sync_mode(SyncMode /*mode*/) override {}

    void set_base_plugin_path(const std::string& /*path*/) override {}

    EngineReturnStatus send_rt_event_to_processor(const RtEvent& /*event*/) override
    {
        got_rt_event = true;
        return EngineReturnStatus::OK;
    }

    dispatcher::BaseEventDispatcher* event_dispatcher() override
    {
        return &_event_dispatcher;
    }

    const BaseProcessorContainer* processor_container() override
    {
        return &_processor_container;
    }

    bool process_called{false};
    bool got_event{false};
    bool got_rt_event{false};

    Transport* transport() override
    {
        return &_transport;
    }

private:
    EventDispatcherMockup _event_dispatcher;
    ProcessorContainerMockup _processor_container;

    Transport _transport;
    RtEventFifo<10> _rt_event_output;
};

// TODO: Should this really be here, or is it too specific for the engine_mockup scope,
//   thus needing its own file?
class DummyMidiFrontend : public sushi::internal::midi_frontend::BaseMidiFrontend
{
public:
    DummyMidiFrontend() : BaseMidiFrontend(nullptr) {}

    ~DummyMidiFrontend() override {}

    bool init() override {return true;}
    void run()  override {}
    void stop() override {}

    void send_midi(int input, MidiDataByte /*data*/, Time /*timestamp*/) override
    {
        _sent = true;
        _input = input;
    }

    bool midi_sent_on_input(int input)
    {
        if (_sent && input == _input)
        {
            _sent = false;
            return true;
        }
        return false;
    }

private:
    bool _sent{false};
    int  _input;
};

#endif //SUSHI_ENGINE_MOCKUP_H
