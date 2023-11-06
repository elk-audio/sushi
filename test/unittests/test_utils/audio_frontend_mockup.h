#ifndef SUSHI_AUDIO_FRONTEND_MOCKUP_H
#define SUSHI_AUDIO_FRONTEND_MOCKUP_H

#include "audio_frontends/base_audio_frontend.h"

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::audio_frontend;

class AudioFrontendMockup : public BaseAudioFrontend
{
public:
    AudioFrontendMockup() : BaseAudioFrontend(nullptr) {}
    AudioFrontendMockup(internal::engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    void cleanup() override {}

    void run() override {}

    void pause([[maybe_unused]] bool enabled) override {}
};


#endif //SUSHI_AUDIO_FRONTEND_MOCKUP_H
