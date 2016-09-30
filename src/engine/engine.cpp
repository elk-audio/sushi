#include "engine.h"
#include "plugin_interface.h"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/equalizer_plugin.h"

#include <algorithm>

namespace sushi_engine {

void set_up_processing_graph(std::vector<std::vector<std::unique_ptr<AudioProcessorBase>>> &graph, unsigned int sample_rate)
{
    /* Set up identical left and right channels with 2 hardcoded plugins each*/
    AudioProcessorConfig config;
    config.sample_rate = sample_rate;

    std::unique_ptr<AudioProcessorBase> unit_l(new passthrough_plugin::PassthroughPlugin());
    unit_l->init(config);
    graph[LEFT].push_back(std::move(unit_l));

    std::unique_ptr<AudioProcessorBase> gain_l(new gain_plugin::GainPlugin());
    gain_l->init(config);
    graph[LEFT].push_back(std::move(gain_l));

    std::unique_ptr<AudioProcessorBase> unit_r(new passthrough_plugin::PassthroughPlugin());
    unit_r->init(config);
    graph[RIGHT].push_back(std::move(unit_r));

    std::unique_ptr<AudioProcessorBase> gain_r(new gain_plugin::GainPlugin());
    gain_r->init(config);
    graph[RIGHT].push_back(std::move(gain_r));
}

void process_channel_graph(std::vector<std::unique_ptr<AudioProcessorBase>> &channel, float* in, float* out)
{
    float buffer[AUDIO_CHUNK_SIZE];
    float* in_tmp = in;
    float* out_tmp = buffer;
    for (auto &n : channel)
    {
        n->process(in_tmp, out_tmp);
        in_tmp = out_tmp;
    }
    std::copy(out_tmp, out_tmp + AUDIO_CHUNK_SIZE, out);

    //std::for_each(channel.begin(), channel.end(), [](AudioProcessorBase* p){p->process(in_tmp, out_tmp); in_tmp = out_tmp; });
}


SushiEngine::SushiEngine(unsigned int sample_rate) : EngineBase::EngineBase(sample_rate)
{
    set_up_processing_graph(_audio_graph, _sample_rate);
}


SushiEngine::~SushiEngine()
{
}


void SushiEngine::process_chunk(SushiBuffer *buffer)
{
    /* process left and right separately */
    process_channel_graph(_audio_graph[LEFT], buffer->left_in, buffer->left_out);
    process_channel_graph(_audio_graph[RIGHT], buffer->right_in, buffer->right_out);
}




} // sushi_engine