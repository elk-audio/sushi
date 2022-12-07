#include <CoreAudio/AudioHardware.h>

OSStatus AudioDeviceCreateIOProcID(AudioObjectID inDevice,
                                   AudioDeviceIOProc inProc,
                                   void* inClientData,
                                   AudioDeviceIOProcID* outIOProcID)
{
    return kAudioHardwareNoError;
}

OSStatus AudioDeviceDestroyIOProcID(AudioObjectID inDevice,
                                    AudioDeviceIOProcID inIOProcID)
{
    return kAudioHardwareNoError;
}

OSStatus AudioDeviceStart(AudioObjectID inDevice,
                          AudioDeviceIOProcID inProcID)
{
    return kAudioHardwareNoError;
}

OSStatus AudioDeviceStop(AudioObjectID inDevice,
                         AudioDeviceIOProcID inProcID)
{
    return kAudioHardwareNoError;
}

OSStatus AudioObjectGetPropertyData(AudioObjectID inObjectID,
                                    const AudioObjectPropertyAddress* inAddress,
                                    UInt32 inQualifierDataSize,
                                    const void* __nullable inQualifierData,
                                    UInt32* ioDataSize,
                                    void* outData)
{
    return kAudioHardwareNoError;
}

OSStatus AudioObjectSetPropertyData(AudioObjectID inObjectID,
                                    const AudioObjectPropertyAddress* inAddress,
                                    UInt32 inQualifierDataSize,
                                    const void* __nullable inQualifierData,
                                    UInt32 inDataSize,
                                    const void* inData)
{
    return kAudioHardwareNoError;
}

OSStatus AudioObjectGetPropertyDataSize(AudioObjectID inObjectID,
                                        const AudioObjectPropertyAddress* inAddress,
                                        UInt32 inQualifierDataSize,
                                        const void* __nullable inQualifierData,
                                        UInt32* outDataSize)
{
    return kAudioHardwareNoError;
}

Boolean AudioObjectHasProperty(AudioObjectID inObjectID,
                               const AudioObjectPropertyAddress* inAddress)
{
    return false;
}

OSStatus AudioObjectIsPropertySettable(AudioObjectID inObjectID,
                                       const AudioObjectPropertyAddress* inAddress,
                                       Boolean* outIsSettable)
{
    return false;
}
