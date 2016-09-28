/**
 * @brief Gain plugin example
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef GAIN_PLUGIN_H
#define GAIN_PLUGIN_H

#include "plugin_interface.h"
namespace gain_plugin {

enum gain_parameter_id
{
    GAIN = 1
};

class GainPlugin : public AudioProcessorBase
{
public:
    GainPlugin();
    ~GainPlugin();
    AudioProcessorStatus init(const AudioProcessorConfig& configuration) override ;

    void set_parameter(unsigned int parameter_id, float value) override ;

    void process(const float *in_buffer, float *out_buffer) override;

private:
    AudioProcessorConfig _configuration;
    float _gain;
};

}// namespace gain_plugin
#endif // GAIN_PLUGIN_H