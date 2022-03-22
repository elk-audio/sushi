#ifndef SUSHI_AUDIO_FRONTEND_MOCKUP_H
#define SUSHI_AUDIO_FRONTEND_MOCKUP_H

#include "audio_frontends/base_audio_frontend.h"

using namespace sushi;
using namespace sushi::audio_frontend;

class AudioFrontendMockup : public audio_frontend::BaseAudioFrontend
{
public:
    AudioFrontendMockup() : BaseAudioFrontend(nullptr) {}
    AudioFrontendMockup(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    void cleanup() override {}

    void run() override {}

    void pause([[maybe_unused]] bool enabled) override {}
};


#endif //SUSHI_AUDIO_FRONTEND_MOCKUP_H
