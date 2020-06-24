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
 * @brief Class for wrapping the 2 data sets (rt and non-rt) needed for realtime
 *        safe operation of Audio and Cv/Gate connections
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CONNECTION_ROUTER_H
#define SUSHI_CONNECTION_ROUTER_H

#include <vector>
#include <mutex>
#include <cassert>

#include <twine/twine.h>

#include <library/rt_event_pipe.h>

namespace sushi {

template <typename T>
class ConnectionStorage
{
public:
    ConnectionStorage(int max_connections) : _max_connections(max_connections)
    {
        _connections_rt.reserve(max_connections);
    }

    const std::vector<T>& connections_rt() const
    {
        assert(twine::is_current_thread_realtime());
        return _connections_rt;
    }

    std::vector<T> connections()
    {
        assert(twine::is_current_thread_realtime() == false);
        std::scoped_lock<std::mutex> lock(_non_rt_lock);
        auto copy = _connections;
        return copy;
    }

    bool add(const T& element, bool realtime_running)
    {
        assert(twine::is_current_thread_realtime() == false);
        std::scoped_lock<std::mutex> lock(_non_rt_lock);
        if (_connections.size() < _max_connections)
        {
            _connections.push_back(element);
            if (realtime_running == false)
            {
                _connections_rt.push_back(element);
            }
            return true;
        }
        return false;
    }

    bool add_rt(const T& element)
    {
        assert(twine::is_current_thread_realtime());
        assert(_connections_rt.size() < _max_connections);
        _connections_rt.push_back(element);
        return true;
    }

    bool remove(const T& pattern, bool realtime_running)
    {
        assert(twine::is_current_thread_realtime() == false);
        std::scoped_lock<std::mutex> lock(_non_rt_lock);
        auto original_size = _connections.size();
        _connections.erase(std::remove(_connections.begin(), _connections.end(), pattern), _connections.end());
        if (realtime_running == false)
        {
            _connections_rt.erase(std::remove(_connections_rt.begin(), _connections_rt.end(), pattern), _connections_rt.end());
        }
        return !(original_size == _connections.size());
    }

    bool remove_rt(const T& pattern)
    {
        assert(twine::is_current_thread_realtime());
        auto original_size = _connections.size();
        _connections_rt.erase(std::remove(_connections_rt.begin(), _connections_rt.end(), pattern), _connections_rt.end());
        return !(original_size == _connections.size());
    }

private:
    std::vector<T> _connections;
    std::vector<T> _connections_rt;
    size_t         _max_connections;
    std::mutex     _non_rt_lock;
};

} // sushi
#endif //SUSHI_CONNECTION_ROUTER_H
