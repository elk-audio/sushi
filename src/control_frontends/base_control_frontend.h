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

#include "library/plugin_events.h"
#include "library/event_fifo.h"
#include "engine/engine.h"

namespace sushi {
namespace control_frontend {

class BaseControlFrontend
{
public:
    BaseControlFrontend(EventFifo* queue, engine::BaseEngine* engine) :_engine(engine),
                                                                       _queue(queue) {}

    virtual ~BaseControlFrontend() {};

    virtual void run() = 0;

    void send_parameter_change_event(ObjectId processor, ObjectId parameter, float value);

    void send_keyboard_event(ObjectId processor, EventType type, int note, float value);

    void add_chain(const std::string& name, int channels);

    void delete_chain(const std::string& name);

    void add_processor(const std::string& chain, const std::string& uid, const std::string& name,
                       const std::string& file, engine::PluginType type);

    void delete_processor(const std::string& chain, const std::string& name);
protected:
    engine::BaseEngine* _engine;

private:
    bool _stop_engine();

    EventFifo* _queue;
};

}; // namespace control_frontend
}; // namespace sushi

#endif //SUSHI_BASE_CONTROL_FRONTEND_H
