#include "gmock/gmock.h"

// Included for the declarations and data types,
// used in PortAudioFrontend, which this mock allows testing.
#include <portaudio.h>

#ifndef SUSHI_TEST_PORTAUDIO_MOCKUP_H
#define SUSHI_TEST_PORTAUDIO_MOCKUP_H

class MockPortAudio
{
public:
    MOCK_METHOD(PaError, Pa_Initialize, ());
    MOCK_METHOD(PaError, Pa_Terminate, ());
    MOCK_METHOD(const char*, Pa_GetErrorText, (PaError));
    MOCK_METHOD(int, Pa_GetDeviceCount, ());
    MOCK_METHOD(int, Pa_GetDefaultInputDevice, ());
    MOCK_METHOD(int, Pa_GetDefaultOutputDevice, ());
    MOCK_METHOD(const PaDeviceInfo*, Pa_GetDeviceInfo, (int));
    MOCK_METHOD(PaError, Pa_IsFormatSupported, (const PaStreamParameters* input,
                                                const PaStreamParameters* output,
                                                double samplerate));
    MOCK_METHOD(PaTime, Pa_GetStreamTime, (PaStream*));
    MOCK_METHOD(PaError, Pa_IsStreamActive, (PaStream*));
    MOCK_METHOD(PaError, Pa_OpenStream, (PaStream** stream,
                                         const PaStreamParameters *inputParameters,
                                         const PaStreamParameters *outputParameters,
                                         double sampleRate,
                                         unsigned long framesPerBuffer,
                                         PaStreamFlags streamFlags,
                                         PaStreamCallback *streamCallback,
                                         void *userData ));
    MOCK_METHOD(PaError, Pa_StartStream, (PaStream*));
    MOCK_METHOD(PaError, Pa_StopStream, (PaStream*));
    MOCK_METHOD(const PaStreamInfo*, Pa_GetStreamInfo, (PaStream*));
};

extern std::unique_ptr<MockPortAudio> mockPortAudio;

PaError Pa_Initialize();

PaError Pa_Terminate();

const char* Pa_GetErrorText(PaError error);

int Pa_GetDeviceCount();

int Pa_GetDefaultInputDevice();

int Pa_GetDefaultOutputDevice();

const PaDeviceInfo* Pa_GetDeviceInfo(int device_index);

PaError Pa_IsFormatSupported(const PaStreamParameters* input,
                             const PaStreamParameters* output,
                             double samplerate);

PaTime Pa_GetStreamTime(PaStream* stream);

PaError Pa_IsStreamActive(PaStream* stream);

PaError Pa_OpenStream(PaStream** stream,
                       const PaStreamParameters *inputParameters,
                       const PaStreamParameters *outputParameters,
                       double sampleRate,
                       unsigned long framesPerBuffer,
                       PaStreamFlags streamFlags,
                       PaStreamCallback *streamCallback,
                       void *userData);

PaError Pa_StartStream(PaStream* stream);

PaError Pa_StopStream(PaStream* stream);

const PaStreamInfo* Pa_GetStreamInfo(PaStream* stream);

#endif