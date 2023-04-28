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
#include <type_traits>
#include <cassert>

#include <twine/twine.h>

#include <library/rt_event_pipe.h>

namespace sushi {

template <typename T>
class ConnectionStorage
{
public:
    explicit ConnectionStorage(int max_connections) : _capacity(max_connections)
    {
        _items_rt.reserve(max_connections);
    }

    /**
     * @brief Get the current items, called from rt threads only, should not be called
     *        be called concurrently with add_rt()
     * @return A reference to a vector of elements in the container
     */
    const std::vector<T>& connections_rt() const
    {
        assert(twine::is_current_thread_realtime());
        return _items_rt;
    }

    /**
     * @brief Get the current elements, called from not-rt threads only. Returns a copy of the
     *        items currently in the container
     * @return A vector of elements in the container
     */
    std::vector<T> connections() const
    {
        assert(twine::is_current_thread_realtime() == false);
        std::scoped_lock<std::mutex> lock(_non_rt_lock);
        auto copy = _items;
        return copy;
    }

    /**
     * @brief Add an element to the container. Should only be called from a non-rt thread
     * @param element   The element to add to the container.
     * @param add_to_rt If true, also adds the element to the rt-part of the container.
     *                  This should only be set to true if there are no concurrent calls from a
     *                  rt thread to the container. If set to false, add_rt() needs to be called
     *                  from an rt thread afterwards.
     * @return True if the element was successfully added, false if the max capacity was reached.
     */
    bool add(const T& element, bool add_to_rt)
    {
        assert(twine::is_current_thread_realtime() == false);
        std::scoped_lock<std::mutex> lock(_non_rt_lock);
        if (_items.size() < _capacity)
        {
            _items.push_back(element);
            if (add_to_rt)
            {
                _items_rt.push_back(element);
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Add an element to the rt part of the container. Should only be called from an rt thread
     * @param element   The element to add to the container.
     * @return True if the element was successfully added, false if the max capacity was reached.
     */
    bool add_rt(const T& element)
    {
        assert(twine::is_current_thread_realtime());
        assert(_items_rt.size() < _capacity);
        _items_rt.push_back(element);
        return true;
    }

    /**
     * @brief Remove an element from the container. Should only be called from a non-rt thread
     * @param pattern   Elements matching this pattern will be removed.
     * @param remove_from_rt If true, also removes the element from the rt-part of the container.
     *                       This should only be set to true if there are no concurrent calls from a
     *                       rt thread to the container. If set to false, remove_rt() needs to be
     *                       called from an rt thread afterwards.
     * @return True if the element was found and successfully deleted.
     */
    bool remove(const T& pattern, bool remove_from_rt)
    {
        assert(twine::is_current_thread_realtime() == false);
        std::scoped_lock<std::mutex> lock(_non_rt_lock);
        auto original_size = _items.size();
        _items.erase(std::remove(_items.begin(), _items.end(), pattern), _items.end());
        if (remove_from_rt)
        {
            _items_rt.erase(std::remove(_items_rt.begin(), _items_rt.end(), pattern), _items_rt.end());
        }
        return (original_size != _items.size());
    }

    /**
     * @brief Remove an element from the rt part of the container. Should only be called from an rt thread
     * @param pattern   Elements matching this pattern will be removed.
     * @return True if the element was found and successfully deleted.
     */
    bool remove_rt(const T& pattern)
    {
        assert(twine::is_current_thread_realtime());
        auto original_size = _items_rt.size();
        _items_rt.erase(std::remove(_items_rt.begin(), _items_rt.end(), pattern), _items_rt.end());
        return (original_size != _items_rt.size());
    }

    size_t capacity() const
    {
        return _capacity;
    }

private:
    // For the mutable rt-operations to be guaranteed rt-safe (no allocations) these must hold
    static_assert(std::is_nothrow_copy_constructible<T>::value);
    static_assert(std::is_nothrow_destructible<T>::value);

    std::vector<T>      _items;
    std::vector<T>      _items_rt;
    size_t              _capacity;
    mutable std::mutex  _non_rt_lock;
};

} // sushi
#endif //SUSHI_CONNECTION_ROUTER_H
