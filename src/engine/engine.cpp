#include "engine.h"
#include "plugin_interface.h"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/equalizer_plugin.h"

#include <algorithm>
#include <cstring>

namespace sushi_engine {

SushiBuffer::SushiBuffer(const unsigned int size) :
    _size(size)
{
    left_in = new float[size];
    right_in = new float[size];
    left_out = new float[size];
    right_out = new float[size];
    clear();
}

SushiBuffer::~SushiBuffer()
{
    delete left_in;
    delete right_in;
    delete left_out;
    delete right_out;
}

void SushiBuffer::clear()
{
    memset(left_in, 0, sizeof(float) * _size);
    memset(right_in, 0, sizeof(float) * _size);
    memset(left_out, 0, sizeof(float) * _size);
    memset(right_out, 0, sizeof(float) * _size);
}

void SushiBuffer::input_from_interleaved(const float *interleaved_buf)
{
    float* lin = left_in;
    float* rin = right_in;
    for (unsigned int n=0; n<_size; n++)
    {
        *lin++ = *interleaved_buf++;
        *rin++ = *interleaved_buf++;
    }
}

void SushiBuffer::output_to_interleaved(float *interleaved_buf)
{
    float* lout = left_out;
    float* rout = right_out;
    for (unsigned int n=0; n<_size; n++)
    {
        *interleaved_buf++ = *lout++;
        *interleaved_buf++ = *rout++;
    }
}

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
    // gain_r->set_parameter(gain_plugin::GAIN, 2.0f);
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