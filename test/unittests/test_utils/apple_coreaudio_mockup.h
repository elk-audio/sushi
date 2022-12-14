#ifndef SUSHI_APPLE_COREAUDIO_MOCKUP_H
#define SUSHI_APPLE_COREAUDIO_MOCKUP_H

#include <CoreAudio/AudioHardware.h>

#include <gmock/gmock.h>

class AppleAudioHardwareMockup
{
public:
    MOCK_METHOD(Boolean, AudioObjectHasProperty, (AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address));
    MOCK_METHOD(OSStatus, AudioObjectGetPropertyData, (AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32 qualifier_data_size, const void* qualifier_data, UInt32* data_size, void* data));
    MOCK_METHOD(OSStatus, AudioObjectSetPropertyData, (AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32 qualifier_data_size, const void* qualifier_data, UInt32 data_size, const void* data));
    MOCK_METHOD(OSStatus, AudioObjectGetPropertyDataSize, (AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, UInt32 qualifier_data_size, const void* qualifier_data, UInt32* out_data_size));
    MOCK_METHOD(OSStatus, AudioObjectIsPropertySettable, (AudioObjectID audio_object_id, const AudioObjectPropertyAddress* address, Boolean* out_is_settable));

    MOCK_METHOD(OSStatus, AudioDeviceCreateIOProcID, (AudioObjectID audio_object_id, AudioDeviceIOProc io_proc, void* client_data, AudioDeviceIOProcID* io_proc_id));
    MOCK_METHOD(OSStatus, AudioDeviceDestroyIOProcID, (AudioObjectID audio_object_id, AudioDeviceIOProcID proc_id));
    MOCK_METHOD(OSStatus, AudioDeviceStart, (AudioObjectID audio_object_id, AudioDeviceIOProcID proc_id));
    MOCK_METHOD(OSStatus, AudioDeviceStop, (AudioObjectID audio_object_id, AudioDeviceIOProcID proc_id));

    static AppleAudioHardwareMockup* instance;
};


#endif// SUSHI_APPLE_COREAUDIO_MOCKUP_H
