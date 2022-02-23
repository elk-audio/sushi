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
    MockOscInterface(int receive_port, int send_port) : BaseOscMessenger(receive_port, send_port) {}
    ~MockOscInterface() override {}

    MOCK_METHOD(bool,
                init,
                (),
                (override));

    MOCK_METHOD(int,
                run,
                (),
                (override));

    MOCK_METHOD(int,
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
};

#endif // SUSHI_MOCK_OSC_INTERFACE_H
