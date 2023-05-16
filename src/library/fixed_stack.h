/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @brief Simple, non-thread safe stack with a limited maximum size,
 *        implemented through an array.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_FIXED_STACK_H
#define SUSHI_FIXED_STACK_H

#include <array>
#include <cassert>

namespace sushi {

template<typename T, size_t storage_capacity>
class FixedStack
{
public:

    /**
     * @brief Push a new element
     *
     * @param element Element to be pushed
     *
     * @return True if success (stack not full)
     */
    bool push(const T& element)
    {
        if (full())
        {
            return false;
        }

        _data[++_head] = element;
        return true;
    }

    /**
     * @brief Pop the head of the stack
     *
     * @param element Output: filled with the popped head
     *
     * @return True if success (stack not empty)
     */
    bool pop(T& element)
    {
        if (empty())
        {
            return false;
        }

        element = _data[_head--];
        return true;
    }

    /**
     * @brief Return the current n. of elements in the stack
     *
     * @return Stack length
     */
    [[nodiscard]] int size() const
    {
        return (_head + 1);
    }

    /**
     * @brief Check if stack is empty
     */
    [[nodiscard]] bool empty() const
    {
        return (_head == -1);
    }

    /**
     * @brief Check if stack is full
     */
    [[nodiscard]] bool full() const
    {
        return (_head == (storage_capacity-1));
    }

private:
    std::array<T, storage_capacity> _data;

    int _head{-1};
};

}// namespace sushi

#endif //SUSHI_FIXED_STACK_H
