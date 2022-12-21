#include "gtest/gtest.h"

#include "audio_frontends/apple_coreaudio/apple_coreaudio_utils.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"// Ignore Apple nonsense
#include "../test_utils/apple_coreaudio_mockup.h"
#pragma clang diagnostic pop

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

TEST(AppleCoreAudioUtils, audio_object_property_address_equality)
{
    AudioObjectPropertyAddress lhs{1, 2, 3};
    AudioObjectPropertyAddress rhs{1, 2, 3};

    EXPECT_EQ(lhs, rhs);

    lhs.mSelector = 0;
    EXPECT_NE(lhs, rhs);
}

TEST(AppleCoreAudioUtils, cf_string_to_std_string)
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
    using apple_coreaudio::AudioObject::property_changed;
    using apple_coreaudio::AudioObject::set_property;
    using apple_coreaudio::AudioObject::set_property_data;

    MOCK_METHOD(void, property_changed, (const AudioObjectPropertyAddress& address));
};

TEST_F(AppleCoreAudioFrontendTest, get_audio_object_id)
{
    CustomAudioObject audio_object0(0);
    EXPECT_EQ(audio_object0.get_audio_object_id(), 0);

    CustomAudioObject audio_object1(1);
    EXPECT_EQ(audio_object1.get_audio_object_id(), 1);
}

TEST_F(AppleCoreAudioFrontendTest, is_valid)
{
    // Zero is not considered a valid object ID.
    CustomAudioObject audio_object0(0);
    EXPECT_FALSE(audio_object0.is_valid());

    // Anything higher than zero is considered a valid object ID.
    CustomAudioObject audio_object1(1);
    EXPECT_TRUE(audio_object1.is_valid());
}

TEST_F(AppleCoreAudioFrontendTest, has_property)
{
    CustomAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));
    EXPECT_FALSE(audio_object.has_property({0, 0, 0}));

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_TRUE(audio_object.has_property({0, 0, 0}));
}

TEST_F(AppleCoreAudioFrontendTest, is_property_settable)
{
    CustomAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectIsPropertySettable).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, Boolean* out_is_settable) {
        *out_is_settable = false;
        return kAudioHardwareNoError;
    });
    EXPECT_FALSE(audio_object.is_property_settable({0, 0, 0}));

    EXPECT_CALL(_mock, AudioObjectIsPropertySettable).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, Boolean* out_is_settable) {
        *out_is_settable = true;
        return kAudioHardwareNoError;
    });
    EXPECT_TRUE(audio_object.is_property_settable({0, 0, 0}));
}

TEST_F(AppleCoreAudioFrontendTest, get_property_data_size)
{
    CustomAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
        *out_data_size = sizeof(UInt32);
        return kAudioHardwareNoError;
    });
    EXPECT_EQ(audio_object.get_property_data_size({1,1,1}), sizeof(UInt32));
}

TEST_F(AppleCoreAudioFrontendTest, get_property_data)
{
    CustomAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
        *data_size = sizeof(UInt32);
        *static_cast<UInt32*>(out_data) = 5;
        return kAudioHardwareNoError;
    });
    UInt32 data;
    EXPECT_EQ(audio_object.get_property_data({1,1,1}, sizeof(UInt32), &data), sizeof(UInt32));
}

TEST_F(AppleCoreAudioFrontendTest, set_property_data)
{
    CustomAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectSetPropertyData).WillOnce(Return(kAudioHardwareNoError));

    UInt32 data;
    EXPECT_TRUE(audio_object.set_property_data({1,1,1}, sizeof(UInt32), &data));
}

TEST_F(AppleCoreAudioFrontendTest, get_property)
{
    CustomAudioObject audio_object(2);

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
        *out_data_size = sizeof(UInt32);
        return kAudioHardwareNoError;
    });
    EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
        *data_size = sizeof(UInt32);
        *static_cast<UInt32*>(out_data) = 5;
        return kAudioHardwareNoError;
    });
    EXPECT_EQ(audio_object.get_property<UInt32>({1, 1, 1}), 5);

    {// If the property has a data size which is not equal to the size of the data type
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
            *out_data_size = sizeof(UInt32) + 1;
            return kAudioHardwareNoError;
        });
        EXPECT_FALSE(audio_object.get_property<UInt32>(2, {1, 1, 1}));
    }

    {// If the property get data returns an invalid data size
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
            *out_data_size = sizeof(UInt32);
            return kAudioHardwareNoError;
        });
        EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
            *data_size = sizeof(UInt32) + 1;
            *static_cast<UInt32*>(out_data) = 5;
            return kAudioHardwareNoError;
        });
        EXPECT_FALSE(audio_object.get_property<UInt32>(2, {1, 1, 1}));
    }
}

TEST_F(AppleCoreAudioFrontendTest, set_property)
{
    CustomAudioObject audio_object(2);

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_CALL(_mock, AudioObjectIsPropertySettable).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, Boolean* out_is_settable) {
        *out_is_settable = true;
        return kAudioHardwareNoError;
    });
    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
        *out_data_size = sizeof(UInt32);
        return kAudioHardwareNoError;
    });
    EXPECT_CALL(_mock, AudioObjectSetPropertyData).WillOnce(Return(kAudioHardwareNoError));
    EXPECT_TRUE(audio_object.set_property<UInt32>({1, 1, 1}, 5));

    {// If the object has no property, expect false
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));
        EXPECT_FALSE(audio_object.set_property<UInt32>(2, {1, 1, 1}, 5));
    }

    {// If property is not settable, expect false
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectIsPropertySettable).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, Boolean* out_is_settable) {
            *out_is_settable = false;
            return kAudioHardwareNoError;
        });
        EXPECT_FALSE(audio_object.set_property<UInt32>(2, {1, 1, 1}, 5));
    }

    {// If the data size does not match the size of the type, expect false
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectIsPropertySettable).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, Boolean* out_is_settable) {
            *out_is_settable = true;
            return kAudioHardwareNoError;
        });
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
            *out_data_size = sizeof(UInt32) + 1;
            return kAudioHardwareNoError;
        });
        EXPECT_FALSE(audio_object.set_property<UInt32>(2, {1, 1, 1}, 5));
    }
}

TEST_F(AppleCoreAudioFrontendTest, get_cf_string_property)
{
    CustomAudioObject audio_object(2);

    CFStringRef string_ref = CFSTR("SomeTestString");

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
        *out_data_size = sizeof(CFStringRef);
        return kAudioHardwareNoError;
    });
    EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([&string_ref](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
        *data_size = sizeof(CFStringRef);
        *static_cast<CFStringRef*>(out_data) = string_ref;
        return kAudioHardwareNoError;
    });
    EXPECT_EQ(audio_object.get_cfstring_property({1, 1, 1}), "SomeTestString");

    // When the object doesn't have a CFString property at given address, it should return an empty string.
    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));
    EXPECT_EQ(audio_object.get_cfstring_property({1, 1, 1}), "");
}

TEST_F(AppleCoreAudioFrontendTest, get_property_array)
{
    CustomAudioObject audio_object(2);

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
        *out_data_size = 3 * sizeof(UInt32);
        return kAudioHardwareNoError;
    });
    EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
        *data_size = 3 * sizeof(UInt32);
        static_cast<UInt32*>(out_data)[0] = 1;
        static_cast<UInt32*>(out_data)[1] = 2;
        static_cast<UInt32*>(out_data)[2] = 3;
        return kAudioHardwareNoError;
    });

    EXPECT_EQ(audio_object.get_property_array<UInt32>({1, 1, 1}), std::vector<UInt32>({1, 2, 3}));

    {// If the object has no property at given address, expect an empty vector.
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));

        std::vector<UInt32> vector;
        EXPECT_FALSE(audio_object.get_property_array<UInt32>(2, {1, 1, 1}, vector));
        EXPECT_TRUE(vector.empty());
    }

    {// If the property reports a data size which is not a multiple of sizeof(Type), then expect an empty array
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
            *out_data_size = 3 * sizeof(UInt32) + 1;
            return kAudioHardwareNoError;
        });
        std::vector<UInt32> vector;
        EXPECT_FALSE(audio_object.get_property_array<UInt32>(2, {1, 1, 1}, vector));
        EXPECT_TRUE(vector.empty());
    }
}

TEST_F(AppleCoreAudioFrontendTest, add_property_listener)
{
    AudioObjectID audio_object_id(2);
    testing::StrictMock<CustomAudioObject> audio_object(audio_object_id);

    AudioObjectPropertyListenerProc listener_proc;
    void* client_data = nullptr;

    EXPECT_CALL(_mock, AudioObjectAddPropertyListener).WillOnce([&listener_proc, &client_data](AudioObjectID, const AudioObjectPropertyAddress*, AudioObjectPropertyListenerProc inListener, void* inClientData) {
        listener_proc = inListener;
        client_data = inClientData;
        return kAudioHardwareNoError;
    });

    EXPECT_TRUE(audio_object.add_property_listener({1, 1, 1}));

    EXPECT_CALL(audio_object, property_changed);

    AudioObjectPropertyAddress pa{1, 1, 1};
    EXPECT_EQ(listener_proc(audio_object_id, 1, &pa, client_data), kAudioHardwareNoError);

    // When no client data is provided the listener function should return an error
    EXPECT_EQ(listener_proc(audio_object_id, 1, &pa, nullptr), kAudioHardwareBadObjectError);

    // When the wrong object id is given, the listener function should return an error
    EXPECT_EQ(listener_proc(audio_object_id + 1, 1, &pa, client_data), kAudioHardwareBadObjectError);

    EXPECT_CALL(_mock, AudioObjectAddPropertyListener);

    // Adding the same property will not add ake AudioObject to add another listener, but will return true.
    EXPECT_TRUE(audio_object.add_property_listener({1, 1, 1}));

    EXPECT_TRUE(audio_object.add_property_listener({2, 2, 2}));

    // When the AudioObject goes out of scope, property listeners should be removed
    EXPECT_CALL(_mock, AudioObjectRemovePropertyListener).Times(2);
}
