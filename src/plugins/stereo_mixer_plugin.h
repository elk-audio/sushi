#ifndef SUSHI_STEREO_MIXER_PLUGIN_H
#define SUSHI_STEREO_MIXER_PLUGIN_H

#include "library/internal_plugin.h"

namespace sushi {
namespace stereo_mixer_plugin {

class StereoMixerPlugin : public InternalPlugin
{
public:
    StereoMixerPlugin(HostControl host_control);

    void process_audio(const ChunkSampleBuffer& in_buffer,ChunkSampleBuffer& out_buffer) override;

private:
    FloatParameterValue* _left_pan;
    FloatParameterValue* _left_gain;
    FloatParameterValue* _left_invert_phase;

    FloatParameterValue* _right_pan;
    FloatParameterValue* _right_gain;
    FloatParameterValue* _right_invert_phase;
};

} // namespace stereo_mixer_plugin
} // namespace sushi


#endif // SUSHI_STEREO_MIXER_PLUGIN_H
