/**
 * @brief Gain plugin example
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef GAIN_PLUGIN_H
#define GAIN_PLUGIN_H

#include "library/internal_plugin.h"

namespace sushi {
namespace gain_plugin {

class GainPlugin : public InternalPlugin
{
public:
    GainPlugin();

    ~GainPlugin();


    const std::string unique_id() override
    {
        return std::string("sushi.testing.gain");
    }

    // void process_event(BaseEvent* /*event*/) {}

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    FloatStompBoxParameter* _gain_parameter;
};

}// namespace gain_plugin
}// namespace sushi
#endif // GAIN_PLUGIN_H