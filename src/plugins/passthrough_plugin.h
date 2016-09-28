/**
 * @brief Example unit gain plugin.
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Passes audio unprocessed from input to output
 */

#ifndef PASSTHROUGH_PLUGIN_H
#define PASSTHROUGH_PLUGIN_H

#include "plugin_interface.h"
namespace passthrough_plugin {


class PassthroughPlugin : public AudioProcessorBase
{
public:
    PassthroughPlugin();
    ~PassthroughPlugin();
    AudioProcessorStatus init(const AudioProcessorConfig& configuration) override ;

    void set_parameter(unsigned int parameter_id, float value) override ;

    void process(const float *in_buffer, float *out_buffer) override;
};

}// namespace passthrough_plugin

#endif //PASSTHROUGH_PLUGIN_H
