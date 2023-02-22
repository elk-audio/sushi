#include "gtest/gtest.h"

#define protected public

#include "audio_frontends/apple_coreaudio/apple_coreaudio_system_object.h"
#include "audio_frontends/apple_coreaudio/apple_coreaudio_utils.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness" // Ignore Apple nonsense
#include "../test_utils/apple_coreaudio_mockup.h"
#pragma clang diagnostic pop

#include "audio_frontends/apple_coreaudio/apple_coreaudio_object.h"

#include "audio_frontends/apple_coreaudio_frontend.cpp"

using ::testing::NiceMock;
using ::testing::Return;

class TestAppleCoreAudioFrontend : public ::testing::Test
{
protected:
    void SetUp() override
    {
        AppleAudioHardwareMockup::instance = &_mock;
    }

    void TearDown() override
    {
    }

    void expect_calls_to_get_cf_string_property(AudioObjectID expected_audio_object_id, const AudioObjectPropertyAddress& expected_addr, const CFStringRef& cf_string_ref)
    {
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([expected_audio_object_id, &expected_addr](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
            EXPECT_EQ(audio_object_id, expected_audio_object_id);
            EXPECT_EQ(*address, expected_addr);
            *out_data_size = sizeof(CFStringRef);
            return kAudioHardwareNoError;
        });
        EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([expected_audio_object_id, &cf_string_ref, &expected_addr](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* data_size, void* out_data) {
            EXPECT_EQ(audio_object_id, expected_audio_object_id);
            EXPECT_EQ(*address, expected_addr);
            *data_size = sizeof(CFStringRef);
            *static_cast<CFStringRef*>(out_data) = cf_string_ref;
            return kAudioHardwareNoError;
        });
    }

    template<class DataType>
    void expect_calls_to_set_property(AudioObjectID expected_audio_object_id, const AudioObjectPropertyAddress& expected_address)
    {
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectIsPropertySettable).WillOnce([expected_audio_object_id, &expected_address](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, Boolean* out_is_settable) {
            EXPECT_EQ(audio_object_id, expected_audio_object_id);
            EXPECT_EQ(*address, expected_address);
            *out_is_settable = true;
            return kAudioHardwareNoError;
        });
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([expected_audio_object_id, &expected_address](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
            EXPECT_EQ(audio_object_id, expected_audio_object_id);
            EXPECT_EQ(*address, expected_address);
            *out_data_size = sizeof(DataType);
            return kAudioHardwareNoError;
        });
        EXPECT_CALL(_mock, AudioObjectSetPropertyData).WillOnce(Return(kAudioHardwareNoError));
    }

    template<class DataType>
    void expect_calls_to_get_property(AudioObjectID expected_audio_object_id, const AudioObjectPropertyAddress& expected_address, const DataType& return_value)
    {
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));

        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([expected_audio_object_id, &expected_address](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
            EXPECT_EQ(audio_object_id, expected_audio_object_id);
            EXPECT_EQ(*address, expected_address);
            *out_data_size = sizeof(DataType);
            return kAudioHardwareNoError;
        });
        EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([&return_value](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
            *data_size = sizeof(DataType);
            *static_cast<DataType*>(out_data) = return_value;
            return kAudioHardwareNoError;
        });
    }

    void expect_calls_for_getting_output_device_name(AudioObjectID expected_audio_object_id,
                                                     const AudioObjectPropertyAddress& expected_address,
                                                     const CFStringRef& cf_string_ref,
                                                     const CFStringRef& return_value)
    {
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillRepeatedly(Return(true));

        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
                EXPECT_EQ(audio_object_id, kAudioObjectSystemObject);
                AudioObjectPropertyAddress expected{kAudioHardwarePropertyDevices,
                                                    kAudioObjectPropertyScopeGlobal,
                                                    kAudioObjectPropertyElementMain};
                EXPECT_EQ(*address, expected);
                *out_data_size = 3 * sizeof(UInt32);
                return kAudioHardwareNoError;
            })
            .WillOnce([expected_audio_object_id, &expected_address](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
                EXPECT_EQ(audio_object_id, expected_audio_object_id);
                EXPECT_EQ(*address, expected_address);
                *out_data_size = sizeof(CFStringRef);
                return kAudioHardwareNoError;
            })
            .WillOnce([expected_audio_object_id, &expected_address](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
                EXPECT_EQ(audio_object_id, expected_audio_object_id);
                EXPECT_EQ(*address, expected_address);
                *out_data_size = sizeof(CFStringRef);
                return kAudioHardwareNoError;
            });

        EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* data_size, void* out_data) {
                EXPECT_EQ(audio_object_id, kAudioObjectSystemObject);
                AudioObjectPropertyAddress expected{kAudioHardwarePropertyDevices,
                                                    kAudioObjectPropertyScopeGlobal,
                                                    kAudioObjectPropertyElementMain};
                EXPECT_EQ(*address, expected);
                *data_size = sizeof(UInt32);
                static_cast<UInt32*>(out_data)[0] = 1;
                return kAudioHardwareNoError;
            })
            .WillOnce([expected_audio_object_id, &cf_string_ref, &expected_address](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* data_size, void* out_data) {
                EXPECT_EQ(audio_object_id, expected_audio_object_id);
                EXPECT_EQ(*address, expected_address);
                *data_size = sizeof(CFStringRef);
                *static_cast<CFStringRef*>(out_data) = cf_string_ref;
                return kAudioHardwareNoError;
            })
            .WillOnce([expected_audio_object_id, &return_value, &expected_address](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* data_size, void* out_data) {
                EXPECT_EQ(audio_object_id, expected_audio_object_id);
                EXPECT_EQ(*address, expected_address);
                *data_size = sizeof(CFStringRef);
                *static_cast<CFStringRef*>(out_data) = return_value;
                return kAudioHardwareNoError;
            });
    }

    testing::StrictMock<AppleAudioHardwareMockup> _mock;
};

TEST_F(TestAppleCoreAudioFrontend, audio_object_property_address_equality)
{
    AudioObjectPropertyAddress lhs{1, 2, 3};
    AudioObjectPropertyAddress rhs{1, 2, 3};

    EXPECT_EQ(lhs, rhs);

    lhs.mSelector = 0;
    EXPECT_NE(lhs, rhs);
}

TEST_F(TestAppleCoreAudioFrontend, cf_string_to_std_string)
{
    auto std_string = apple_coreaudio::cf_string_to_std_string(CFSTR("TestString"));
    EXPECT_EQ(std_string, "TestString");
}

/**
 * Little helper class which publicly exposes protected methods.
 */
class MockAudioObject : public apple_coreaudio::AudioObject
{
public:
    explicit MockAudioObject(AudioObjectID audio_object_id) : AudioObject(audio_object_id) {}

    MOCK_METHOD(void, property_changed, (const AudioObjectPropertyAddress& address));
};

TEST_F(TestAppleCoreAudioFrontend, AudioObject_get_audio_object_id)
{
    MockAudioObject audio_object0(0);
    EXPECT_EQ(audio_object0.get_audio_object_id(), 0);

    MockAudioObject audio_object1(1);
    EXPECT_EQ(audio_object1.get_audio_object_id(), 1);
}

TEST_F(TestAppleCoreAudioFrontend, AudioObject_is_valid)
{
    // Zero is not considered a valid object ID.
    MockAudioObject audio_object0(0);
    EXPECT_FALSE(audio_object0.is_valid());

    // Anything higher than zero is considered a valid object ID.
    MockAudioObject audio_object1(1);
    EXPECT_TRUE(audio_object1.is_valid());
}

TEST_F(TestAppleCoreAudioFrontend, AudioObject_has_property)
{
    MockAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));
    EXPECT_FALSE(audio_object.has_property({0, 0, 0}));

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_TRUE(audio_object.has_property({0, 0, 0}));
}

TEST_F(TestAppleCoreAudioFrontend, AudioObject_is_property_settable)
{
    MockAudioObject audio_object(0);

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

TEST_F(TestAppleCoreAudioFrontend, AudioObject_get_property_data_size)
{
    MockAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
        *out_data_size = sizeof(UInt32);
        return kAudioHardwareNoError;
    });
    EXPECT_EQ(audio_object.get_property_data_size({1, 1, 1}), sizeof(UInt32));
}

TEST_F(TestAppleCoreAudioFrontend, AudioObject_get_property_data)
{
    MockAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
        *data_size = sizeof(UInt32);
        *static_cast<UInt32*>(out_data) = 5;
        return kAudioHardwareNoError;
    });
    UInt32 data;
    EXPECT_EQ(audio_object.get_property_data({1, 1, 1}, sizeof(UInt32), &data), sizeof(UInt32));
}

TEST_F(TestAppleCoreAudioFrontend, AudioObject_set_property_data)
{
    MockAudioObject audio_object(0);

    EXPECT_CALL(_mock, AudioObjectSetPropertyData).WillOnce(Return(kAudioHardwareNoError));

    UInt32 data;
    EXPECT_TRUE(audio_object.set_property_data({1, 1, 1}, sizeof(UInt32), &data));
}

TEST_F(TestAppleCoreAudioFrontend, AudioObject_get_property)
{
    MockAudioObject audio_object(2);

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

    { // If the property has a data size which is not equal to the size of the data type
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
            *out_data_size = sizeof(UInt32) + 1;
            return kAudioHardwareNoError;
        });
        EXPECT_FALSE(audio_object.get_property<UInt32>(2, {1, 1, 1}));
    }

    { // If the property get data returns an invalid data size
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

TEST_F(TestAppleCoreAudioFrontend, AudioObject_set_property)
{
    MockAudioObject audio_object(2);

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

    { // If the object has no property, expect false
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));
        EXPECT_FALSE(audio_object.set_property<UInt32>(2, {1, 1, 1}, 5));
    }

    { // If property is not settable, expect false
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectIsPropertySettable).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, Boolean* out_is_settable) {
            *out_is_settable = false;
            return kAudioHardwareNoError;
        });
        EXPECT_FALSE(audio_object.set_property<UInt32>(2, {1, 1, 1}, 5));
    }

    { // If the data size does not match the size of the type, expect false
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

TEST_F(TestAppleCoreAudioFrontend, AudioObject_get_cf_string_property)
{
    MockAudioObject audio_object(2);

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

TEST_F(TestAppleCoreAudioFrontend, AudioObject_get_property_array)
{
    MockAudioObject audio_object(2);

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

    { // If the object has no property at given address, expect an empty vector.
        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));

        std::vector<UInt32> vector;
        EXPECT_FALSE(audio_object.get_property_array<UInt32>(2, {1, 1, 1}, vector));
        EXPECT_TRUE(vector.empty());
    }

    { // If the property reports a data size which is not a multiple of sizeof(Type), then expect an empty array
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

TEST_F(TestAppleCoreAudioFrontend, AudioObject_add_property_listener)
{
    AudioObjectID audio_object_id(2);
    testing::StrictMock<MockAudioObject> audio_object(audio_object_id);

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

TEST_F(TestAppleCoreAudioFrontend, AudioSystemObject_get_audio_devices)
{
    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
        EXPECT_EQ(audio_object_id, kAudioObjectSystemObject);
        AudioObjectPropertyAddress expected{kAudioHardwarePropertyDevices,
                                            kAudioObjectPropertyScopeGlobal,
                                            kAudioObjectPropertyElementMain};
        EXPECT_EQ(*address, expected);
        *out_data_size = 3 * sizeof(UInt32);
        return kAudioHardwareNoError;
    });
    EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* data_size, void* out_data) {
        EXPECT_EQ(audio_object_id, kAudioObjectSystemObject);
        AudioObjectPropertyAddress expected{kAudioHardwarePropertyDevices,
                                            kAudioObjectPropertyScopeGlobal,
                                            kAudioObjectPropertyElementMain};
        EXPECT_EQ(*address, expected);
        *data_size = 3 * sizeof(UInt32);
        static_cast<UInt32*>(out_data)[0] = 2;
        static_cast<UInt32*>(out_data)[1] = 3;
        static_cast<UInt32*>(out_data)[2] = 4;
        return kAudioHardwareNoError;
    });

    auto devices = apple_coreaudio::AudioSystemObject::get_audio_devices();

    EXPECT_EQ(devices.size(), 3);
    EXPECT_EQ(devices[0].get_audio_object_id(), 2);
    EXPECT_EQ(devices[1].get_audio_object_id(), 3);
    EXPECT_EQ(devices[2].get_audio_object_id(), 4);
}

TEST_F(TestAppleCoreAudioFrontend, AudioSystemObject_get_default_device_id)
{
    { // For input
        AudioObjectPropertyAddress expected_addr{kAudioHardwarePropertyDefaultInputDevice,
                                                 kAudioObjectPropertyScopeGlobal,
                                                 kAudioObjectPropertyElementMain};

        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([&expected_addr](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
            EXPECT_EQ(audio_object_id, kAudioObjectSystemObject);
            EXPECT_EQ(*address, expected_addr);
            *out_data_size = sizeof(UInt32);
            return kAudioHardwareNoError;
        });
        EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([&expected_addr](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* data_size, void* out_data) {
            EXPECT_EQ(audio_object_id, kAudioObjectSystemObject);
            EXPECT_EQ(*address, expected_addr);
            *data_size = sizeof(UInt32);
            *static_cast<UInt32*>(out_data) = 5;
            return kAudioHardwareNoError;
        });
        EXPECT_EQ(apple_coreaudio::AudioSystemObject::get_default_device_id(true), 5);
    }

    { // For output
        AudioObjectPropertyAddress expected_addr{kAudioHardwarePropertyDefaultOutputDevice,
                                                 kAudioObjectPropertyScopeGlobal,
                                                 kAudioObjectPropertyElementMain};

        EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
        EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([&expected_addr](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* out_data_size) {
            EXPECT_EQ(audio_object_id, kAudioObjectSystemObject);
            EXPECT_EQ(*address, expected_addr);
            *out_data_size = sizeof(UInt32);
            return kAudioHardwareNoError;
        });
        EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([&expected_addr](AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32, const void*, UInt32* data_size, void* out_data) {
            EXPECT_EQ(audio_object_id, kAudioObjectSystemObject);
            EXPECT_EQ(*address, expected_addr);
            *data_size = sizeof(UInt32);
            *static_cast<UInt32*>(out_data) = 5;
            return kAudioHardwareNoError;
        });
        EXPECT_EQ(apple_coreaudio::AudioSystemObject::get_default_device_id(false), 5);
    }
}

static OSStatus dummy_audio_device_io_proc(AudioObjectID,
                                           const AudioTimeStamp*,
                                           const AudioBufferList*,
                                           const AudioTimeStamp*,
                                           AudioBufferList*,
                                           const AudioTimeStamp*,
                                           void*) { return kAudioHardwareNoError; }

auto assign_dummy_io_proc = [](AudioObjectID, AudioDeviceIOProc, void*, AudioDeviceIOProcID* proc_id) {
    *proc_id = dummy_audio_device_io_proc; // AudioDevice needs this in order to make stop_io() do something.
    return kAudioHardwareNoError;
};

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_start_io)
{
    apple_coreaudio::AudioDevice::AudioCallback callback;

    {
        apple_coreaudio::AudioDevice invalid_audio_device(0);
        EXPECT_FALSE(invalid_audio_device.start_io(&callback))
                << "Refuse to start an audio device when the AudioObjectID is invalid.";
    }

    apple_coreaudio::AudioDevice audio_device(5);

    EXPECT_FALSE(audio_device.start_io(nullptr))
            << "When the callback is nullptr, the audio device should not start.";

    EXPECT_CALL(_mock, AudioDeviceCreateIOProcID).WillOnce(assign_dummy_io_proc);
    EXPECT_CALL(_mock, AudioDeviceStart);
    EXPECT_CALL(_mock, AudioObjectAddPropertyListener);

    EXPECT_TRUE(audio_device.start_io(&callback));

    // At destruction the audio device should properly close and stop etc.
    EXPECT_CALL(_mock, AudioObjectRemovePropertyListener);
    EXPECT_CALL(_mock, AudioDeviceStop);
    EXPECT_CALL(_mock, AudioDeviceDestroyIOProcID);
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_stop_io)
{
    apple_coreaudio::AudioDevice::AudioCallback callback;
    apple_coreaudio::AudioDevice audio_device(5);

    EXPECT_CALL(_mock, AudioDeviceCreateIOProcID).WillOnce(assign_dummy_io_proc);
    EXPECT_CALL(_mock, AudioDeviceStart);
    EXPECT_CALL(_mock, AudioObjectAddPropertyListener);
    audio_device.start_io(&callback);

    EXPECT_CALL(_mock, AudioDeviceStop);
    EXPECT_CALL(_mock, AudioDeviceDestroyIOProcID);
    audio_device.stop_io();

    // Expected at device destruction
    EXPECT_CALL(_mock, AudioObjectRemovePropertyListener);
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_get_name)
{
    apple_coreaudio::AudioDevice audio_device(5);

    auto device_name = CFSTR("device_name");

    expect_calls_to_get_cf_string_property(5, {kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain},
                                           device_name);
    EXPECT_EQ(audio_device.get_name(), "device_name");

    apple_coreaudio::AudioDevice invalid_audio_device(0);
    EXPECT_EQ(invalid_audio_device.get_name(), "");
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_get_uid)
{
    apple_coreaudio::AudioDevice audio_device(5);

    auto device_uid = CFSTR("device_uid");

    expect_calls_to_get_cf_string_property(5, {kAudioDevicePropertyDeviceUID, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain},
                                           device_uid);
    EXPECT_EQ(audio_device.get_uid(), "device_uid");

    apple_coreaudio::AudioDevice invalid_audio_device(0);
    EXPECT_EQ(invalid_audio_device.get_uid(), "");
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_get_num_channels)
{
    apple_coreaudio::AudioDevice invalid_audio_device(0);
    EXPECT_EQ(invalid_audio_device.get_num_channels(true), -1);

    // Return -1 when the object has no stream configuration property.
    apple_coreaudio::AudioDevice audio_device(5);
    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(false));
    EXPECT_EQ(audio_device.get_num_channels(true), -1);

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillOnce(Return(true));
    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
        *out_data_size = sizeof(AudioBufferList) + 2 * sizeof(AudioBuffer);
        return kAudioHardwareNoError;
    });
    EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillOnce([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
        *data_size = sizeof(AudioBufferList) + 2 * sizeof(AudioBuffer);
        auto* audio_buffer_lists = static_cast<AudioBufferList*>(out_data);
        audio_buffer_lists[0].mNumberBuffers = 3;
        audio_buffer_lists[0].mBuffers[0].mNumberChannels = 1;
        audio_buffer_lists[0].mBuffers[1].mNumberChannels = 2;
        audio_buffer_lists[0].mBuffers[2].mNumberChannels = 3;
        return kAudioHardwareNoError;
    });
    // By default, an audio device selects the first stream only, so while there are multiple streams (buffers) available we expect a channel count of 1.
    EXPECT_EQ(audio_device.get_num_channels(true), 1);
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_set_buffer_frame_size)
{
    apple_coreaudio::AudioDevice invalid_audio_device(0);
    EXPECT_FALSE(invalid_audio_device.set_buffer_frame_size(512));

    apple_coreaudio::AudioDevice audio_device(5);

    expect_calls_to_set_property<UInt32>(5, {kAudioDevicePropertyBufferFrameSize,
                                             kAudioObjectPropertyScopeGlobal,
                                             kAudioObjectPropertyElementMain});

    EXPECT_TRUE(audio_device.set_buffer_frame_size(512));
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_set_nominal_sample_rate)
{
    apple_coreaudio::AudioDevice invalid_audio_device(0);
    EXPECT_FALSE(invalid_audio_device.set_nominal_sample_rate(48000.0));

    apple_coreaudio::AudioDevice audio_device(5);

    expect_calls_to_set_property<double>(5, {kAudioDevicePropertyNominalSampleRate,
                                             kAudioObjectPropertyScopeGlobal,
                                             kAudioObjectPropertyElementMain});

    EXPECT_TRUE(audio_device.set_nominal_sample_rate(48000.0));
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_get_nominal_sample_rate)
{
    apple_coreaudio::AudioDevice invalid_audio_device(0);
    EXPECT_EQ(invalid_audio_device.get_nominal_sample_rate(), 0.0);

    apple_coreaudio::AudioDevice audio_device(5);

    expect_calls_to_get_property<double>(5, {kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain}, 48000.0);

    EXPECT_EQ(audio_device.get_nominal_sample_rate(), 48000.0);
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_get_device_latency)
{
    apple_coreaudio::AudioDevice invalid_audio_device(0);
    EXPECT_EQ(invalid_audio_device.get_device_latency(true), 0);

    apple_coreaudio::AudioDevice audio_device(5);

    expect_calls_to_get_property<UInt32>(5, {kAudioDevicePropertyLatency, kAudioObjectPropertyScopeInput, kAudioObjectPropertyElementMain}, 320);

    EXPECT_EQ(audio_device.get_device_latency(true), 320);

    expect_calls_to_get_property<UInt32>(5, {kAudioDevicePropertyLatency, kAudioObjectPropertyScopeOutput, kAudioObjectPropertyElementMain}, 330);

    EXPECT_EQ(audio_device.get_device_latency(false), 330);
}

TEST_F(TestAppleCoreAudioFrontend, AudioDevice_get_sttream_latency)
{
    apple_coreaudio::AudioDevice invalid_audio_device(0);
    EXPECT_EQ(invalid_audio_device.get_stream_latency(0, true), 0);

    // Because GMOCK can't have different expectations set for the same call we creatively use the expectations below
    // for both getting the array of streams ids (of size 1) and the latency property of the stream with index 0.
    // The side effect of this is that the latency is 1, which is also the id of the first stream. But for testing this is fine.

    EXPECT_CALL(_mock, AudioObjectHasProperty).WillRepeatedly(Return(true));
    EXPECT_CALL(_mock, AudioObjectGetPropertyDataSize).WillRepeatedly([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* out_data_size) {
        *out_data_size = sizeof(UInt32);
        return kAudioHardwareNoError;
    });
    EXPECT_CALL(_mock, AudioObjectGetPropertyData).WillRepeatedly([](AudioObjectID, const AudioObjectPropertyAddress*, UInt32, const void*, UInt32* data_size, void* out_data) {
        *data_size = sizeof(UInt32);
        static_cast<UInt32*>(out_data)[0] = 1; // This is the id of stream with index 0
        return kAudioHardwareNoError;
    });

    apple_coreaudio::AudioDevice audio_device(1);

    EXPECT_EQ(audio_device.get_stream_latency(0, true), 1);

    // Test out of bounds stream
    EXPECT_EQ(audio_device.get_stream_latency(1, true), 0);
}

TEST_F(AppleCoreAudioFrontendTest, get_coreaudio_output_device_name_valid_argument_test)
{
    AudioObjectID expected_audio_object_id = 1;
    auto cf_string_ref = CFSTR("device_uid");
    auto device_name = CFSTR("device_name");

    expect_calls_for_getting_output_device_name(expected_audio_object_id,
                                                {kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal,
                                                 kAudioObjectPropertyElementMain},
                                                cf_string_ref,
                                                device_name);

    std::optional<std::string> apple_coreaudio_output_device_uid = "device_uid";

    auto fetched_device_name = sushi::audio_frontend::get_coreaudio_output_device_name(apple_coreaudio_output_device_uid);

    EXPECT_TRUE(fetched_device_name.has_value());

    auto std_string = apple_coreaudio::cf_string_to_std_string(device_name);

    EXPECT_EQ(fetched_device_name.value(), std_string);
}

TEST_F(AppleCoreAudioFrontendTest, get_coreaudio_output_device_name_invalid_argument_test)
{
    AudioObjectID expected_audio_object_id = 1;
    auto cf_string_ref = CFSTR("device_uid");
    auto device_name = CFSTR("device_name");

    AudioObjectPropertyAddress address {kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};

    expect_calls_for_getting_output_device_name(expected_audio_object_id,
                                                address,
                                                cf_string_ref,
                                                device_name);

    std::optional<std::string> apple_coreaudio_output_device_uid = "INVALID";

    auto fetched_device_name = sushi::audio_frontend::get_coreaudio_output_device_name(apple_coreaudio_output_device_uid);

    apple_coreaudio::AudioDevice audio_device(1);
    audio_device.get_name(); // Doing this to satisfy a requirement in the "expect" method.

    EXPECT_FALSE(fetched_device_name.has_value());
}