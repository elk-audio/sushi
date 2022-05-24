/**
 * @brief Mock object for Sushi class
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_MOCK_SUSHI_H
#define SUSHI_MOCK_SUSHI_H

#include <gmock/gmock.h>

#include "include/sushi/sushi_interface.h"

namespace sushi
{
    void init_logger(const SushiOptions& /*options*/) {}
    std::string to_string(InitStatus /*init_status*/) { return ""; }
}

using namespace sushi;

class MockSushi : public AbstractSushi
{
public:
    MOCK_METHOD(InitStatus,
                init,
                (const SushiOptions& options),
                (override));

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

    MOCK_METHOD(audio_frontend::PassiveFrontend*,
                audio_frontend,
                (),
                (override));

    MOCK_METHOD(void,
                set_sample_rate,
                (float sample_rate),
                (override));

    MOCK_METHOD(midi_frontend::PassiveMidiFrontend*,
                midi_frontend,
                (),
                (override));

    MOCK_METHOD(engine::AudioEngine*,
                audio_engine,
                (),
                (override));
};

#endif //SUSHI_MOCK_SUSHI_H
