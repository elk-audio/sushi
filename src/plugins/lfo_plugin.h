/**
 * @brief Simple Cv control plugin example
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef LFO_PLUGIN_H
#define LFO_PLUGIN_H

#include "library/internal_plugin.h"

namespace sushi {
namespace lfo_plugin {

class LfoPlugin : public InternalPlugin
{
public:
    LfoPlugin(HostControl host_control);

    ~LfoPlugin();

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    float _phase{0};
    float _buffers_per_second;
    FloatParameterValue* _freq_parameter;
    FloatParameterValue* _out_parameter;
};

}// namespace lfo_plugin
}// namespace sushi
#endif // LFO_PLUGIN_H