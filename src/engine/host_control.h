/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Object passed to processors to allow them access to the engine
 *        to query things like time, tempo and to post non rt events.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_HOST_CONTROL_H
#define SUSHI_HOST_CONTROL_H

#include <string>

#include "base_event_dispatcher.h"
#include "engine/transport.h"

namespace sushi {

class HostControl
{
public:
    HostControl(dispatcher::BaseEventDispatcher* event_dispatcher, engine::Transport* transport) : _event_dispatcher(event_dispatcher),
                                                                                                   _transport(transport)
    {}

    /**
     * @brief Post an event into the dispatcher's queue
     *
     * @param event Non-owning pointer to the event
     */
    void post_event(Event* event)
    {
        _event_dispatcher->post_event(event);
    }

    /**
     * @brief Get the engine's transport interface
     */
    const engine::Transport* transport()
    {
        return _transport;
    }

    /**
     * @brief Set an absolute path to be the base for plugin paths
     *
     * @param path Absolute path of the base plugin folder
     */
    void set_base_plugin_path(const std::string& path);

    /**
     * @brief Convert a relative plugin path to an absolute path,
     *        if a base plugin path has been set.
     *
     * @param path Relative path to plugin inside the base plugin folder.
     *             It is the caller's responsibility
     *             to ensure that this is a proper relative path (not starting with "/").
     *
     * @return Absolute path of the plugin
     */
    std::string convert_plugin_path(const std::string& path);

protected:
    dispatcher::BaseEventDispatcher* _event_dispatcher;
    engine::Transport*               _transport;
    std::string                      _base_plugin_path;
};

} // end namespace sushi
#endif //SUSHI_HOST_CONTROL_H
