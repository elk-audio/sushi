#include "engine.h"

#include "plugin_interface.h"
#include "logging.h"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/equalizer_plugin.h"

#include <algorithm>
#include <cstring>

namespace sushi {
namespace engine {

MIND_GET_LOGGER;

void set_up_processing_graph(eastl::vector<eastl::vector<std::unique_ptr<StompBox>>> &graph, int sample_rate)
{
    /* Set up identical left and right channels with 2 hardcoded plugins each*/
    StompBoxConfig config;
    config.sample_rate = sample_rate;

    std::unique_ptr<StompBox> unit_l(new passthrough_plugin::PassthroughPlugin());
    unit_l->init(config);
    graph[LEFT].push_back(std::move(unit_l));

    std::unique_ptr<StompBox> gain_l(new gain_plugin::GainPlugin());
    gain_l->init(config);
    graph[LEFT].push_back(std::move(gain_l));

    std::unique_ptr<StompBox> unit_r(new passthrough_plugin::PassthroughPlugin());
    unit_r->init(config);
    graph[RIGHT].push_back(std::move(unit_r));

    std::unique_ptr<StompBox> gain_r(new gain_plugin::GainPlugin());
    gain_r->init(config);
    // gain_r->set_parameter(gain_plugin::GAIN, 2.0f);
    graph[RIGHT].push_back(std::move(gain_r));
}

void AudioEngine::process_channel_graph(eastl::vector<std::unique_ptr<StompBox>> &channel,
                                                    const SampleBuffer<AUDIO_CHUNK_SIZE>& in,
                                                    SampleBuffer<AUDIO_CHUNK_SIZE>& out)
{
    _tmp_bfr_1 = in;

    for (auto &p : channel)
    {
        p->process(&_tmp_bfr_1, &_tmp_bfr_2);
        std::swap(_tmp_bfr_1, _tmp_bfr_2);
    }
    out = _tmp_bfr_1;
}


AudioEngine::AudioEngine(int sample_rate) : BaseEngine::BaseEngine(sample_rate)
{
    set_up_processing_graph(_audio_graph, _sample_rate);
}


AudioEngine::~AudioEngine()
{
}


void AudioEngine::process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* For now, process every channel in in_buffer separately and copy the result to
     * the corresponding channel in out_buffer */

    for (int ch = 0; ch < in_buffer->channel_count(); ++ch)
    {
        if (ch >= static_cast<int>(_audio_graph.size()) || ch >= out_buffer->channel_count())
        {
            MIND_LOG_WARNING("Warning, not all input channels processed, {} out of {} processed", ch, in_buffer->channel_count());
            break;
        }
        _tmp_bfr_in.replace(0, ch, *in_buffer);
        process_channel_graph(_audio_graph[ch],_tmp_bfr_in, _tmp_bfr_out);
        out_buffer->replace(ch, 0, _tmp_bfr_out);
    }
}


} // namespace engine
} // namespace sushi