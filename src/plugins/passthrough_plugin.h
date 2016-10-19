/**
 * @brief Example unit gain plugin.
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Passes audio unprocessed from input to output
 */

#ifndef PASSTHROUGH_PLUGIN_H
#define PASSTHROUGH_PLUGIN_H

#include "plugin_interface.h"

namespace sushi {
namespace passthrough_plugin {

class PassthroughPlugin : public StompBox
{
public:
    PassthroughPlugin();

    ~PassthroughPlugin();

    StompBoxStatus init(const StompBoxConfig &configuration) override;

    void set_parameter(int parameter_id, float value) override;

    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;
};

}// namespace passthrough_plugin
}// namespace sushi
#endif //PASSTHROUGH_PLUGIN_H
