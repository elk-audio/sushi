#include <algorithm>
#include <memory>
#include <random>
#include <tuple>

#include "gtest/gtest.h"

#define private public
#define protected public

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"
#include "library/internal_plugin.h"
#include "library/plugin_registry.h"

#include "plugins/brickworks/bitcrusher_plugin.h"
#include "plugins/brickworks/compressor_plugin.cpp"
#include "plugins/brickworks/bitcrusher_plugin.cpp"
#include "plugins/brickworks/wah_plugin.cpp"
#include "plugins/brickworks/eq3band_plugin.cpp"
#include "plugins/brickworks/phaser_plugin.cpp"
#include "plugins/brickworks/chorus_plugin.cpp"
#include "plugins/brickworks/vibrato_plugin.cpp"
#include "plugins/brickworks/combdelay_plugin.cpp"
#include "plugins/brickworks/flanger_plugin.cpp"
#include "plugins/brickworks/saturation_plugin.cpp"
#include "plugins/brickworks/noise_gate_plugin.cpp"
#include "plugins/brickworks/tremolo_plugin.cpp"
#include "plugins/brickworks/notch_plugin.cpp"
#include "plugins/brickworks/multi_filter_plugin.cpp"
#include "plugins/brickworks/highpass_plugin.cpp"
#include "plugins/brickworks/clip_plugin.cpp"
#include "plugins/brickworks/fuzz_plugin.cpp"
#include "plugins/brickworks/simple_synth_plugin.cpp"

using namespace sushi;

constexpr float TEST_SAMPLERATE = 48000;
constexpr int   TEST_CHANNEL_COUNT = 2;

constexpr int TEST_PROCESS_N_ITERATIONS = 128;



/**
 * @brief Helper function to instantiate internal plugins from a UID
 *        We need to return everything because we can't use google test macros
 *        inside a non-void-returning function
 */
std::tuple<ProcessorReturnCode, ProcessorReturnCode, std::shared_ptr<Processor>> instantiate_plugin(const std::string& uid)
{
    HostControlMockup host_control;
    PluginRegistry registry;

    PluginInfo pinfo = { .uid = uid, .type = PluginType::INTERNAL};

    auto hc = host_control.make_host_control_mockup(TEST_SAMPLERATE);
    auto [processor_status, plugin] = registry.new_instance(pinfo, hc, TEST_SAMPLERATE);

    ProcessorReturnCode init_status = plugin->init(TEST_SAMPLERATE);
    plugin->set_enabled(true);
    plugin->set_input_channels(TEST_CHANNEL_COUNT);
    plugin->set_output_channels(TEST_CHANNEL_COUNT);

    return std::make_tuple(processor_status, init_status, plugin);
}

/**
 * @brief Set a random value for all the parameters of a plugin
 *
 * @param plugin Plugin object
 * @param dev Random number generator to use
 * @param dist Statistical distribution
 */
template<class random_device, class random_dist>
void set_plugin_random_parameters(Processor* plugin,
                                  random_device& dev, random_dist& dist)
{
    for (auto& pd : plugin->all_parameters())
    {
        plugin->process_event(RtEvent::make_parameter_change_event(plugin->id(), 0, pd->id(), dist(dev)));
    }
}

/**
 * @brief Test that silence input produces silence output,
 *        over TEST_PROCESS_N_ITERATIONS while randomly varying parameters between each buffer
 */
void test_fx_plugin_silencein_silenceout(const std::string& uid, float error_margin)
{
    std::ranlux24 rand_dev;
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};

    auto [processor_status, init_status, plugin] = instantiate_plugin(uid);
    ASSERT_EQ(processor_status, ProcessorReturnCode::OK);
    ASSERT_EQ(init_status, ProcessorReturnCode::OK);

    ChunkSampleBuffer in_buffer{TEST_CHANNEL_COUNT};
    ChunkSampleBuffer out_buffer{TEST_CHANNEL_COUNT};

    for (int i = 0; i < TEST_PROCESS_N_ITERATIONS; i++)
    {
        set_plugin_random_parameters(plugin.get(), rand_dev, dist);
        plugin->process_audio(in_buffer, out_buffer);
        test_utils::assert_buffer_value(0.0f, out_buffer, error_margin);
    }
}

/**
 * @brief Test that white noise input does not generate NaNs,
 *        over TEST_PROCESS_N_ITERATIONS while randomly varying parameters between each buffer
 */
void test_fx_plugin_noisein_notnan(const std::string& uid)
{
    std::ranlux24 rand_dev;
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};

    auto [processor_status, init_status, plugin] = instantiate_plugin(uid);
    ASSERT_EQ(processor_status, ProcessorReturnCode::OK);
    ASSERT_EQ(init_status, ProcessorReturnCode::OK);

    ChunkSampleBuffer in_buffer{TEST_CHANNEL_COUNT};
    ChunkSampleBuffer out_buffer{TEST_CHANNEL_COUNT};

    for (int i = 0; i < TEST_PROCESS_N_ITERATIONS; i++)
    {
        set_plugin_random_parameters(plugin.get(), rand_dev, dist);
        plugin->process_audio(in_buffer, out_buffer);
        test_utils::assert_buffer_not_nan(out_buffer);
    }
}

/**
 * @brief Test that plugin does not write outside its output buffer boundary,
 *        by feeding it white noise over TEST_PROCESS_N_ITERATIONS 
 *        while randomly varying parameters between each buffer
 */
void test_fx_plugin_buffers_not_overflow(const std::string& uid)
{
    std::ranlux24 rand_dev;
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};

    auto [processor_status, init_status, plugin] = instantiate_plugin(uid);
    ASSERT_EQ(processor_status, ProcessorReturnCode::OK);
    ASSERT_EQ(init_status, ProcessorReturnCode::OK);

    ChunkSampleBuffer in_buffer{TEST_CHANNEL_COUNT+1};
    ChunkSampleBuffer out_buffer{TEST_CHANNEL_COUNT+1};

    for (int i = 0; i < TEST_PROCESS_N_ITERATIONS; i++)
    {
        set_plugin_random_parameters(plugin.get(), rand_dev, dist);
        plugin->process_audio(in_buffer, out_buffer);
        test_utils::assert_buffer_value(0.0f, SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 2, 1));
    }
}


/**
 * @brief Run a series of test on a given plugin class:
 *              - instantiation w. label & UID check
 *              - channel count
 *              - silence input produces silence output, while randomly setting parameters
 *              - white noise input does not produce NaNs, while randomly setting parameters
 */
void test_fx_plugin_instantiation(const std::string& uid,
                                   const std::string& label)
{

    auto [processor_status, init_status, plugin] = instantiate_plugin(uid);
    ASSERT_EQ(processor_status, ProcessorReturnCode::OK);
    ASSERT_EQ(init_status, ProcessorReturnCode::OK);
    ASSERT_EQ(uid, plugin->name());
    ASSERT_EQ(plugin->label(), label);
    ASSERT_EQ(TEST_CHANNEL_COUNT, plugin->output_channels());
    ASSERT_EQ(TEST_CHANNEL_COUNT, plugin->input_channels());
}

// We could have packed more test functions into less TEST cases,
// but then it's hard to figure out what is failing.
// Use a macro to automatically expand one-line into the four test cases

#define BRICKWORKS_PLUGIN_TEST_CASES(testlabel, plugin_uid, plugin_label, error_margin)\
    TEST(TestBrickworksFxPlugins, testlabel##Instantiation)\
    {\
        test_fx_plugin_instantiation(plugin_uid, plugin_label);\
    }\
    TEST(TestBrickworksPlugins, testlabel##SilenceInSilenceOut)\
    {\
        test_fx_plugin_silencein_silenceout(plugin_uid, error_margin);\
    }\
    TEST(TestBrickworksPlugins, testlabel##NoiseInNotNan)\
    {\
        test_fx_plugin_noisein_notnan(plugin_uid);\
    }\
    TEST(TestBrickworksPlugins, testlabel##BuffersDontOverflow)\
    {\
        test_fx_plugin_buffers_not_overflow(plugin_uid);\
    }

BRICKWORKS_PLUGIN_TEST_CASES(Chorus, "sushi.brickworks.chorus", "Chorus", 1.0e-4)
BRICKWORKS_PLUGIN_TEST_CASES(Clip, "sushi.brickworks.clip", "Clip", 1.0e-2f)
BRICKWORKS_PLUGIN_TEST_CASES(CombDelay,"sushi.brickworks.comb_delay", "Comb Delay", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Compressor, "sushi.brickworks.compressor", "Compressor", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Eq3band, "sushi.brickworks.eq3band", "3-band Equalizer", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Flanger, "sushi.brickworks.flanger", "Flanger", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Fuzz, "sushi.brickworks.fuzz", "Fuzz", 1.0e-1f)
BRICKWORKS_PLUGIN_TEST_CASES(HighPass, "sushi.brickworks.highpass", "HighPass", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(MultiFilter, "sushi.brickworks.multi_filter", "MultiFilter", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Notch, "sushi.brickworks.notch", "Notch", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Phaser, "sushi.brickworks.phaser", "Phaser", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Saturation, "sushi.brickworks.saturation", "Saturation", 1.0e-2f)
BRICKWORKS_PLUGIN_TEST_CASES(Tremolo, "sushi.brickworks.tremolo", "Tremolo", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Vibrato, "sushi.brickworks.vibrato", "Vibrato", 1.0e-4f)
BRICKWORKS_PLUGIN_TEST_CASES(Wah, "sushi.brickworks.wah", "Wah", 1.0e-4f)

// the SilenceIn test is tricky for the NoiseGate atm, so we skip it

TEST(TestBrickworksFxPlugins, TestNoiseGateInstantiation)
{
    test_fx_plugin_instantiation("sushi.brickworks.noise_gate", "Noise gate");
}

TEST(TestBrickworksFxPlugins, TestNoiseGateNoiseInNotNaN)
{
    test_fx_plugin_noisein_notnan("sushi.brickworks.noise_gate");
}

TEST(TestBrickworksFxPlugins, TestNoiseGateBuffersDontOverflow)
{
    test_fx_plugin_buffers_not_overflow("sushi.brickworks.noise_gate");
}


// Bitcrusher plugin is an exception because it has one integer parameter,
// and the silence threshold heavily depends on the bit depth parameter

class TestBitcrusherPlugin : public ::testing::Test
{
protected:
    TestBitcrusherPlugin()
    {
        _rand_gen.seed(1234);
    }

    void SetUp()
    {
        _module_under_test = std::make_unique<bitcrusher_plugin::BitcrusherPlugin>(_host_control.make_host_control_mockup());
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        _module_under_test->set_enabled(true);
        _module_under_test->set_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_output_channels(TEST_CHANNEL_COUNT);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }

    void SetRandomParameters()
    {
        _module_under_test->_samplerate_ratio->set_processed(_sr_dist(_rand_gen));
        _module_under_test->_bit_depth->set_processed(_bd_dist(_rand_gen));
    }

    HostControlMockup _host_control;
    std::unique_ptr<bitcrusher_plugin::BitcrusherPlugin> _module_under_test;

    std::ranlux24 _rand_gen;
    std::uniform_real_distribution<float> _sr_dist{0.0f, 1.0f};
    std::uniform_int_distribution<int> _bd_dist{1, 16};
};

TEST_F(TestBitcrusherPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Bitcrusher", _module_under_test->label());
    ASSERT_EQ("sushi.brickworks.bitcrusher", _module_under_test->name());
    ASSERT_EQ(bitcrusher_plugin::BitcrusherPlugin::static_uid(), _module_under_test->uid());
}

TEST_F(TestBitcrusherPlugin, TestSilenceInSilenceOut)
{
    ChunkSampleBuffer in_buffer{TEST_CHANNEL_COUNT};
    ChunkSampleBuffer out_buffer{TEST_CHANNEL_COUNT};

    for (int i = 0; i < TEST_PROCESS_N_ITERATIONS; i++)
    {
        SetRandomParameters();
        _module_under_test->process_audio(in_buffer, out_buffer);
        // Threshold is dependent on bitdepth resolution
        test_utils::assert_buffer_value(0.0f, out_buffer, 1.0f / (1 + _module_under_test->_bit_depth->processed_value()));
    }
}

TEST_F(TestBitcrusherPlugin, TestNoiseInputNotNan)
{
    ChunkSampleBuffer in_buffer{TEST_CHANNEL_COUNT};
    ChunkSampleBuffer out_buffer{TEST_CHANNEL_COUNT};

    for (int i = 0; i < TEST_PROCESS_N_ITERATIONS; i++)
    {
        test_utils::fill_buffer_with_noise(in_buffer);
        SetRandomParameters();
        _module_under_test->process_audio(in_buffer, out_buffer);
        test_utils::assert_buffer_not_nan(out_buffer);
    }
}

