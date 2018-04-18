/**
 * @brief Base class for control frontend
 * @copyright MIND Music Labs AB, Stockholm
 *
 * This module provides run-time control of the audio engine for parameter
 * changes and plugin control.
 */
#ifndef SUSHI_BASE_CONTROL_FRONTEND_H
#define SUSHI_BASE_CONTROL_FRONTEND_H

#include <string>
#include <vector>
#include <mutex>

#include "library/event_interface.h"
#include "engine/engine.h"

namespace sushi {
namespace control_frontend {

class BaseControlFrontend : public EventPoster
{
public:
    BaseControlFrontend(engine::BaseEngine* engine, EventPosterId id) : _engine(engine),
                                                                        _poster_id(id)
    {
        _event_dispatcher = _engine->event_dispatcher();
        _event_dispatcher->register_poster(this);
    }

    virtual ~BaseControlFrontend()
    {
        _event_dispatcher->deregister_poster(this);
    };

    static void completion_callback(void *arg, Event* event, int return_status)
    {
        static_cast<BaseControlFrontend*>(arg)->_completion_callback(event, return_status);
    }

    virtual void run() = 0;

    virtual void stop() = 0;

    void send_parameter_change_event(ObjectId processor, ObjectId parameter, float value);

    void send_string_parameter_change_event(ObjectId processor, ObjectId parameter, const std::string& value);

    void send_keyboard_event(ObjectId processor, KeyboardEvent::Subtype type, int note, float value);

    void send_note_on_event(ObjectId processor, int note, float value);

    void send_note_off_event(ObjectId processor, int note, float value);

    void send_program_change_event(ObjectId processor, int program);

    void send_add_track_event(const std::string &name, int channels);

    void send_remove_track_event(const std::string &name);

    void send_add_processor_event(const std::string &track, const std::string &uid, const std::string &name,
                                  const std::string &file, AddProcessorEvent::ProcessorType type);

    void send_remove_processor_event(const std::string &track, const std::string &name);

    void send_set_tempo_event(float tempo);

    void send_set_time_signature_event(TimeSignature signature);

    void send_set_playing_mode_event(PlayingMode mode);

    void send_set_sync_mode_event(SyncMode mode);

    int poster_id() override {return _poster_id;}

protected:
    virtual void _completion_callback(Event* event, int return_status) = 0;

    void send_with_callback(Event *event);

    engine::BaseEngine* _engine;
    dispatcher::BaseEventDispatcher* _event_dispatcher;
    std::vector<EventId> _sendlist;
    std::mutex _sendlist_mutex;

private:
    EventPosterId _poster_id;
};

}; // namespace control_frontend
}; // namespace sushi

#endif //SUSHI_BASE_CONTROL_FRONTEND_H
