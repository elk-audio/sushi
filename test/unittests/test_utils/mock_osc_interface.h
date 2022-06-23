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

#ifndef SUSHI_MOCK_OSC_INTERFACE_H
#define SUSHI_MOCK_OSC_INTERFACE_H

#include <gmock/gmock.h>

#include "control_frontends/osc_frontend.h"
#include "control_frontends/osc_utils.h"

using namespace sushi;
using namespace sushi::osc;

class MockOscInterface: public BaseOscMessenger
{
public:
    MockOscInterface(int receive_port,
                     int send_port,
                     const std::string& send_ip) : BaseOscMessenger(receive_port,
                                                                    send_port,
                                                                    send_ip) {}

    ~MockOscInterface() override {}

    MOCK_METHOD(bool,
                init,
                (),
                (override));

    MOCK_METHOD(void,
                run,
                (),
                (override));

    MOCK_METHOD(void,
                stop,
                (),
                (override));

    MOCK_METHOD(void*,
                add_method,
                (const char* address_pattern,
                 const char* type_tag_string,
                 OscMethodType type,
                 const void* connection),
                (override));

    MOCK_METHOD(void,
                delete_method,
                (void* method),
                (override));

    MOCK_METHOD(void,
                send,
                (const char* address_pattern, float payload),
                (override));

    MOCK_METHOD(void,
                send,
                (const char* address_pattern, int payload),
                (override));

    MOCK_METHOD(void,
                send,
                (const char* address_pattern, const std::string& payload),
                (override));
};

#endif // SUSHI_MOCK_OSC_INTERFACE_H
