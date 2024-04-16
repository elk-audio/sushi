#ifndef SUSHI_AUDIO_FRONTEND_MOCKUP_H
#define SUSHI_AUDIO_FRONTEND_MOCKUP_H

#include <gmock/gmock.h>

#include "audio_frontends/base_audio_frontend.h"

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::audio_frontend;

class AudioFrontendMockup : public BaseAudioFrontend
{
public:
    AudioFrontendMockup() : BaseAudioFrontend(nullptr) {}

    MOCK_METHOD(void,
                cleanup,
                (),
                (override));

    MOCK_METHOD(void,
                run,
                (),
                (override));

    MOCK_METHOD(void,
                pause,
                (bool paused),
                (override));
};

#endif //SUSHI_AUDIO_FRONTEND_MOCKUP_H
