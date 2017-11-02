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

    virtual void run() = 0;

    virtual void stop() = 0;

    void send_parameter_change_event(ObjectId processor, ObjectId parameter, float value);

    void send_string_parameter_change_event(ObjectId processor, ObjectId parameter, const std::string& value);

    void send_keyboard_event(ObjectId processor, KeyboardEvent::Subtype type, int note, float value);

    void send_note_on_event(ObjectId processor, int note, float value);

    void send_note_off_event(ObjectId processor, int note, float value);

    void add_chain(const std::string& name, int channels);

    void delete_chain(const std::string& name);

    void add_processor(const std::string& chain, const std::string& uid, const std::string& name,
                       const std::string& file, engine::PluginType type);

    void delete_processor(const std::string& chain, const std::string& name);

    int poster_id() override {return _poster_id;}

protected:
    engine::BaseEngine* _engine;
    dispatcher::BaseEventDispatcher* _event_dispatcher;

private:
    EventPosterId _poster_id;
};

}; // namespace control_frontend
}; // namespace sushi

#endif //SUSHI_BASE_CONTROL_FRONTEND_H
