#include "gmock/gmock.h"
#include "portaudio.h"

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

MockPortAudio* mockPortAudio;

PaError Pa_Initialize()
{
    return mockPortAudio->Pa_Initialize();
}

PaError Pa_Terminate()
{
    return mockPortAudio->Pa_Terminate();
}

const char* Pa_GetErrorText(PaError error)
{
    return mockPortAudio->Pa_GetErrorText(error);
}

int Pa_GetDeviceCount()
{
    return mockPortAudio->Pa_GetDeviceCount();
}

int Pa_GetDefaultInputDevice()
{
    return mockPortAudio->Pa_GetDefaultInputDevice();
}

int Pa_GetDefaultOutputDevice()
{
    return mockPortAudio->Pa_GetDefaultOutputDevice();
}

const PaDeviceInfo* Pa_GetDeviceInfo(int device_index)
{
    return mockPortAudio->Pa_GetDeviceInfo(device_index);
}

PaError Pa_IsFormatSupported(const PaStreamParameters* input,
                             const PaStreamParameters* output,
                             double samplerate)
{
    return mockPortAudio->Pa_IsFormatSupported(input, output, samplerate);
}

PaTime Pa_GetStreamTime(PaStream* stream)
{
    return mockPortAudio->Pa_GetStreamTime(stream);
}

PaError Pa_IsStreamActive(PaStream* stream)
{
    return mockPortAudio->Pa_IsStreamActive(stream);
}

PaError Pa_OpenStream(PaStream** stream,
                       const PaStreamParameters *inputParameters,
                       const PaStreamParameters *outputParameters,
                       double sampleRate,
                       unsigned long framesPerBuffer,
                       PaStreamFlags streamFlags,
                       PaStreamCallback *streamCallback,
                       void *userData )
{
    return mockPortAudio->Pa_OpenStream(stream,
                                       inputParameters,
                                       outputParameters,
                                       sampleRate,
                                       framesPerBuffer,
                                       streamFlags,
                                       streamCallback,
                                       userData);
}

PaError Pa_StartStream(PaStream* stream)
{
    return mockPortAudio->Pa_StartStream(stream);
}

PaError Pa_StopStream(PaStream* stream)
{
    return mockPortAudio->Pa_StopStream(stream);
}

const PaStreamInfo* Pa_GetStreamInfo(PaStream* stream)
{
    return mockPortAudio->Pa_GetStreamInfo(stream);
}
