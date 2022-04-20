#include <cmath>

#include "vst2_test_plugin.h"

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
    return new test_utils::Vst2TestPlugin(audioMaster);
}

namespace test_utils {

constexpr char EFFECT_NAME[] = "Test Plugin";
constexpr char VENDOR_NAME[] = "Elk";

constexpr char PROGRAM_1[] = "Program 1";
constexpr char PROGRAM_2[] = "Program 2";
constexpr char PROGRAM_3[] = "Program 3";
constexpr std::array<const char*, 3> PROGRAM_NAMES = {PROGRAM_1, PROGRAM_2, PROGRAM_3};

constexpr char PARAM_1[] = "Gain";
constexpr char PARAM_2[] = "Dummy";
constexpr std::array<const char*, 2> PARAM_NAMES = {PARAM_1, PARAM_2};

constexpr uint8_t NOTE_OFF_PREFIX   = 0b10000000;
constexpr uint8_t NOTE_ON_PREFIX    = 0b10010000;

Vst2TestPlugin::Vst2TestPlugin(audioMasterCallback audioMaster) : AudioEffectX(audioMaster,
                                                                               PROGRAM_NAMES.size(),
                                                                               PARAM_NAMES.size()),
                                                                  _parameters{{1.0f, 1.0f}},
                                                                  _program_no{0},
                                                                  _frequency{0.01},
                                                                  _phase{0},
                                                                  _playing{false}
{
    setNumInputs(2);
    setNumOutputs(2);
    setUniqueID(1234);
    canProcessReplacing();
    isSynth(true);
    programsAreChunks(false);
}

Vst2TestPlugin::~Vst2TestPlugin() = default;

void Vst2TestPlugin::setSampleRate(float sampleRate)
{
    _samplerate = sampleRate;
}

void Vst2TestPlugin::setParameter(VstInt32 index, float value)
{
    if (index < Parameters::MAX_PARAMS)
    {
        _parameters[index] = value;
    }
}

float Vst2TestPlugin::getParameter(VstInt32 index)
{
    if (index < Parameters::MAX_PARAMS)
    {
        return _parameters[index];
    }
    return 0;
}

void Vst2TestPlugin::getParameterName(VstInt32 index, char* label)
{
    if (index < Parameters::MAX_PARAMS && index >= 0)
    {
        vst_strncpy(label, PARAM_NAMES[index], kVstMaxParamStrLen);
    }
}

void Vst2TestPlugin::getParameterDisplay(VstInt32 index, char* text)
{
    switch (index)
    {
        case Parameters::GAIN:
            dB2string(_parameters[Parameters::GAIN], text, kVstMaxParamStrLen);
            break;

        case Parameters::DUMMY:
            float2string(_parameters[Parameters::DUMMY], text, kVstMaxParamStrLen);
            break;

        default:
            vst_strncpy(text, "", kVstMaxParamStrLen);
    }
}

void Vst2TestPlugin::getParameterLabel(VstInt32 index, char*label)
{
    switch (index)
    {
        case Parameters::GAIN:
            vst_strncpy(label, "dB", kVstMaxParamStrLen);
            break;

        default:
            vst_strncpy(label, "", kVstMaxParamStrLen);
    }
}

bool Vst2TestPlugin::getEffectName(char*name)
{
    vst_strncpy(name, EFFECT_NAME, kVstMaxEffectNameLen);
    return true;
}

bool Vst2TestPlugin::getProductString(char*text)
{
    vst_strncpy(text, EFFECT_NAME, kVstMaxProductStrLen);
    return true;
}

bool Vst2TestPlugin::getVendorString(char*text)
{
    vst_strncpy(text, VENDOR_NAME, kVstMaxVendorStrLen);
    return true;
}

VstInt32 Vst2TestPlugin::getVendorVersion()
{
    return 1234;
}

void Vst2TestPlugin::processReplacing(float**inputs, float**outputs, VstInt32 sampleFrames)
{
    float*in1 = inputs[0];
    float*in2 = inputs[1];
    float*out1 = outputs[0];
    float*out2 = outputs[1];

    for (int i = 0; i < sampleFrames; ++i)
    {
        out1[i] = in1[i] * _parameters[Parameters::GAIN];
        out2[i] = in2[i] * _parameters[Parameters::GAIN];
        if (_playing)
        {
            out1[i] += std::cos(_phase * M_PI);
            out2[i] += std::signbit(_phase - 0.5f)? 0.5f : -0.5f;
        }
        _phase = std::fmod(_phase + _frequency, 1.0f);
    }
}

void Vst2TestPlugin::setProgram(VstInt32 program)
{
    if (program < static_cast<int>(PROGRAM_NAMES.size()) && program >= 0)
    {
        _program_no = program;
    }
}

VstInt32 Vst2TestPlugin::getProgram()
{
    return _program_no;
}

void Vst2TestPlugin::getProgramName(char* name)
{
    vst_strncpy(name, PROGRAM_NAMES[_program_no], kVstMaxProgNameLen);
}

bool Vst2TestPlugin::getProgramNameIndexed(VstInt32 /*category*/, VstInt32 index, char* text)
{
    if (index < static_cast<int>(PROGRAM_NAMES.size()) && index >= 0)
    {
        vst_strncpy(text, PROGRAM_NAMES[index], kVstMaxProgNameLen);
        return true;
    }
    return false;
}

VstInt32 Vst2TestPlugin::processEvents(VstEvents* events)
{
    for (int i = 0 ; i < events->numEvents; ++i)
    {
        if (events->events[i]->type == kVstMidiType)
        {
            auto midi_ev = reinterpret_cast<VstMidiEvent*>(events->events[i]);
            uint8_t command = midi_ev->midiData[0] & 0xf0;
            if (command == NOTE_ON_PREFIX)
            {
                _frequency = 440.0f * std::pow(2.0f, (midi_ev->midiData[1] - 69) / 12.0f) / _samplerate;
                _playing = true;
            }
            if (command == NOTE_OFF_PREFIX)
            {
                _playing = false;
            }

        }
    }
    return 0;
}

}