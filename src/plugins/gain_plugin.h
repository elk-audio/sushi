/**
 * @brief Gain plugin example
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef GAIN_PLUGIN_H
#define GAIN_PLUGIN_H

#include "plugin_interface.h"
#include "library/plugin_parameters.h"

namespace sushi {
namespace gain_plugin {

class GainPlugin : public StompBox
{
public:
    GainPlugin();

    ~GainPlugin();

    StompBoxStatus init(const StompBoxConfig &configuration) override;

    std::string unique_id() const override
    {
        return std::string("sushi.testing.gain");
    }
    void process_event(BaseEvent* /*event*/) {}

    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

private:
    StompBoxConfig _configuration;
    FloatStompBoxParameter* _gain_parameter;
};

}// namespace gain_plugin
}// namespace sushi
#endif // GAIN_PLUGIN_H