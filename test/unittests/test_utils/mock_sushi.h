/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Mock object for Sushi class
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_MOCK_SUSHI_H
#define SUSHI_MOCK_SUSHI_H

#include <gmock/gmock.h>

#include "sushi.h"

namespace sushi
{
    void init_logger(const SushiOptions& /*options*/) {}
    std::string to_string(InitStatus /*init_status*/) { return ""; }
}

using namespace sushi;

class MockSushi : public AbstractSushi
{
public:
    MOCK_METHOD(void,
                start,
                (),
                (override));

    MOCK_METHOD(void,
                exit,
                (),
                (override));

    MOCK_METHOD(engine::Controller*,
                controller,
                (),
                (override));

    MOCK_METHOD(void,
                set_sample_rate,
                (float sample_rate),
                (override));

    MOCK_METHOD(float,
                sample_rate,
                (),
                (const override));
};

#endif //SUSHI_MOCK_SUSHI_H
