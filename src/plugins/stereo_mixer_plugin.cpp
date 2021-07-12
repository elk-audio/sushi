#include "stereo_mixer_plugin.h"

namespace sushi {
namespace stereo_mixer_plugin {

StereoMixerPlugin::StereoMixerPlugin(HostControl HostControl): InternalPlugin(HostControl) {}

void StereoMixerPlugin::process_audio(const ChunkSampleBuffer& input_buffer,
                                     ChunkSampleBuffer& output_buffer)
{
    output_buffer.add(input_buffer);
}

} // namespace stereo_mixer_plugin
} // namespace sushi

