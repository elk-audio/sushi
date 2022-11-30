#include "gtest/gtest.h"

#include "audio_frontends/apple_coreaudio_frontend.cpp"

TEST(TestTest, DoSomeInitialTesting)
{
    AudioDevice audio_device(1);

    auto devices = AudioSystemObject::get_audio_devices();
    EXPECT_TRUE(devices.empty());
}
