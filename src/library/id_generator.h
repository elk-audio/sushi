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
 * @brief Unique id generators for processors, parameters, etc.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */
#ifndef SUSHI_ID_GENERATOR_H
#define SUSHI_ID_GENERATOR_H

#include <atomic>

template <typename T>
class BaseIdGenerator
{
public:
    static T new_id()
    {
        static std::atomic<T> counter{0};
        return counter.fetch_add(1);
    }
};

typedef uint32_t ObjectId;

class ProcessorIdGenerator : public BaseIdGenerator<ObjectId>
{ };

typedef uint16_t EventId;

class EventIdGenerator : public BaseIdGenerator<EventId>
{ };
#endif // SUSHI_ID_GENERATOR_H
