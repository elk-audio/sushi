/**
 * @brief Audio processor with event output
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef SUSHI_PEAK_METER_PLUGIN_H
#define SUSHI_PEAK_METER_PLUGIN_H

#include "library/internal_plugin.h"

namespace sushi {
namespace peak_meter_plugin {

constexpr int MAX_METERED_CHANNELS = 2;

class PeakMeterPlugin : public InternalPlugin
{
public:
    PeakMeterPlugin(HostControl host_control);

    virtual ~PeakMeterPlugin() = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    void _update_refresh_interval(float sample_rate);

    FloatParameterValue* _left_level;
    FloatParameterValue* _right_level;
    int _refresh_interval;
    int _sample_count{0};
    float _smoothing_coef{0.0f};
    std::array<float, MAX_METERED_CHANNELS> _smoothed{ {0.0f} };
};

}// namespace peak_meter_plugin
}// namespace sushi

#endif //SUSHI_PEAK_METER_PLUGIN_H
