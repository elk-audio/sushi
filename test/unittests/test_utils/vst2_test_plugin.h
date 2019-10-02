#ifndef SUSHI_VST2_TEST_PLUGIN_H
#define SUSHI_VST2_TEST_PLUGIN_H

#include <array>

#include "public.sdk/source/vst2.x/audioeffectx.h"

/**
 * @brief Vst 2.4 plugin for testing the wrapper implementation. It can work as both as
 * a simple gain control, and as a crude synthesizer, outputting a sine wave on the left
 * channel and a square wave on the right.
 */

namespace test_utils {

enum Parameters
{
    GAIN,
    DUMMY,
    MAX_PARAMS
};

class Vst2TestPlugin : public AudioEffectX
{
public:
    explicit Vst2TestPlugin(audioMasterCallback audioMaster);

    virtual ~Vst2TestPlugin();

    VstInt32 processEvents (VstEvents* events) override;

    void processReplacing(float**inputs, float**outputs, VstInt32 sampleFrames) override;

    void setSampleRate (float sampleRate) override;

    void setParameter(VstInt32 index, float value) override;

    float getParameter(VstInt32 index) override;

    void getParameterLabel(VstInt32 index, char*label) override;

    void getParameterDisplay(VstInt32 index, char*text) override;

    void getParameterName(VstInt32 index, char*text) override;

    void setProgram(VstInt32 program) override;

    VstInt32 getProgram() override;

    void getProgramName(char* name) override;

    bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text) override;

    bool getEffectName(char*name) override;

    bool getVendorString(char*text) override;

    bool getProductString(char*text) override;

    VstInt32 getVendorVersion() override;

private:
    std::array<float, Parameters::MAX_PARAMS> _parameters;
    int _program_no;
    float _samplerate;
    float _frequency;
    float _phase;
    bool _playing;
};

}// namespace test_utils

#endif //SUSHI_VST2_TEST_PLUGIN_H
