#include "portaudio.h"

static int device_count;
static int default_input_device;
static int default_output_device;
static PaDeviceInfo device_infos[2];

static double expected_samplerate;
static double expected_stream_time;
static bool active_stream;

PaError Pa_Initialize()
{
    return -1000;
}

const char* Pa_GetErrorText(PaError error)
{
    return "Test error text";
}

int Pa_GetDeviceCount()
{
    return device_count;
}

int Pa_GetDefaultInputDevice()
{
    return default_input_device;
}

int Pa_GetDefaultOutputDevice()
{
    return default_output_device;
}

const PaDeviceInfo* Pa_GetDeviceInfo(int device_index)
{
    if (device_index < 0)
    {
        return &device_infos[device_index];
    }
    return NULL;
}

PaError Pa_IsFormatSupported(const PaStreamParameters* input,
                             const PaStreamParameters* output,
                             double samplerate)
{
    if (samplerate == expected_samplerate)
    {
        return 0;
    }
    return PaErrorCode::paInvalidSampleRate;
}

PaTime Pa_GetStreamTime(PaStream* stream)
{
    return expected_stream_time;
}

PaError Pa_IsStreamActive(PaStream* stream)
{
    if (active_stream)
    {
        return 0;
    }
    return PaErrorCode::paStreamIsStopped;
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
    return -1;
}

PaError Pa_StartStream(PaStream* stream)
{
    return -1;
}

PaError Pa_StopStream(PaStream* stream)
{
    return -1;
}