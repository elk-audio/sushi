#include "gtest/gtest.h"

#include "audio_frontends/apple_coreaudio_frontend.cpp"

TEST(TestTest, DoSomeInitialTesting)
{
    apple_coreaudio::AudioDevice audio_device(1);

    auto devices = apple_coreaudio::AudioSystemObject::get_audio_devices();
    EXPECT_TRUE(devices.empty());
}
