#include "gtest/gtest.h"

#include "audio_frontends/apple_coreaudio/apple_coreaudio_utils.h"

#include "../test_utils/apple_coreaudio_mockup.h"
#include "audio_frontends/apple_coreaudio/apple_coreaudio_object.h"

using ::testing::NiceMock;
using ::testing::Return;

class AppleCoreAudioFrontendTest : public ::testing::Test
{
protected:
    void SetUp()
    {
        AppleAudioHardwareMockup::instance = &_mock;
    }

    void TearDown()
    {
    }

    testing::StrictMock<AppleAudioHardwareMockup> _mock;
};

TEST(AppleCoreAudioUtils, AudioObjectPropertyAddressEquality)
{
    AudioObjectPropertyAddress lhs{1, 2, 3};
    AudioObjectPropertyAddress rhs{1, 2, 3};

    EXPECT_EQ(lhs, rhs);

    lhs.mSelector = 0;
    EXPECT_NE(lhs, rhs);
}

TEST(AppleCoreAudioUtils, CFStringToStdString)
{
    auto std_string = apple_coreaudio::cf_string_to_std_string(CFSTR("TestString"));
    EXPECT_EQ(std_string, "TestString");
}

/**
 * Little helper class which publicly exposes protected methods.
 */
class CustomAudioObject : public apple_coreaudio::AudioObject
{
public:
    explicit CustomAudioObject(AudioObjectID audio_object_id) : AudioObject(audio_object_id) {}

    using apple_coreaudio::AudioObject::add_property_listener;
    using apple_coreaudio::AudioObject::get_cfstring_property;
    using apple_coreaudio::AudioObject::get_property;
    using apple_coreaudio::AudioObject::get_property_array;
    using apple_coreaudio::AudioObject::get_property_data;
    using apple_coreaudio::AudioObject::get_property_data_size;
    using apple_coreaudio::AudioObject::has_property;
    using apple_coreaudio::AudioObject::is_property_settable;
    using apple_coreaudio::AudioObject::set_property;
    using apple_coreaudio::AudioObject::set_property_data;
};

TEST_F(AppleCoreAudioFrontendTest, HasPropertyStatic)
{
    EXPECT_CALL(_mock, AudioObjectHasProperty).WillRepeatedly(Return(false));
    EXPECT_FALSE(apple_coreaudio::AudioObject::has_property(0, {0, 0, 0}));

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillRepeatedly(Return(true));
    EXPECT_TRUE(apple_coreaudio::AudioObject::has_property(1, {1, 1, 1}));
}

TEST_F(AppleCoreAudioFrontendTest, HasProperty)
{
    CustomAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));
    EXPECT_FALSE(audio_object.has_property({0, 0, 0}));

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_TRUE(audio_object.has_property({0, 0, 0}));
}

TEST_F(AppleCoreAudioFrontendTest, GetAudioObjectId)
{
    CustomAudioObject audio_object0(0);
    EXPECT_EQ(audio_object0.get_audio_object_id(), 0);

    CustomAudioObject audio_object1(1);
    EXPECT_EQ(audio_object1.get_audio_object_id(), 1);
}
