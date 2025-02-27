#include <algorithm>
#include <memory>
#include <random>

#include "gtest/gtest.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"

#include "plugins/brickworks/simple_synth_plugin.cpp"

namespace sushi::internal::simple_synth_plugin
{

class Accessor
{
public:
    explicit Accessor(SimpleSynthPlugin& plugin) : _plugin(plugin) {}

    [[nodiscard]] FloatParameterValue* decay()
    {
        return _plugin._decay;
    }

    [[nodiscard]] FloatParameterValue* release()
    {
        return _plugin._release;
    }

private:
    SimpleSynthPlugin& _plugin;
};

}

using namespace sushi;
using namespace sushi::internal::simple_synth_plugin;

constexpr float TEST_SAMPLERATE = 48000;
constexpr int   TEST_CHANNEL_COUNT = 2;


class TestSimpleSynthPlugin : public ::testing::Test
{
protected:

    void SetUp() override
    {
        _module_under_test = std::make_unique<SimpleSynthPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));

        _accessor = std::make_unique<sushi::internal::simple_synth_plugin::Accessor>(*_module_under_test);

        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_channels(0, TEST_CHANNEL_COUNT);
        _module_under_test->set_enabled(true);
    }

    HostControlMockup _host_control;
    std::unique_ptr<SimpleSynthPlugin> _module_under_test;

    std::unique_ptr<sushi::internal::simple_synth_plugin::Accessor> _accessor;
};

TEST_F(TestSimpleSynthPlugin, TestInstantiation)
{
    ChunkSampleBuffer in_buffer(TEST_CHANNEL_COUNT);
    ChunkSampleBuffer out_buffer(TEST_CHANNEL_COUNT);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);
}

TEST_F(TestSimpleSynthPlugin, TestProcessing)
{
    ChunkSampleBuffer in_buffer(TEST_CHANNEL_COUNT);
    ChunkSampleBuffer out_buffer(TEST_CHANNEL_COUNT);

    RtEvent note_on = RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f);
    _module_under_test->process_event(note_on);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_non_null(out_buffer);

    RtEvent note_off = RtEvent::make_note_off_event(0, 0, 0, 60, 1.0f);
    _module_under_test->process_event(note_off);
    // give some time for release to act
    float total_release = _accessor->decay()->processed_value() + _accessor->release()->processed_value();
    int release_buffers = static_cast<int>((total_release * TEST_SAMPLERATE) / AUDIO_CHUNK_SIZE) + 1;
    for (int i = 0; i < release_buffers; i++)
    {
        _module_under_test->process_audio(in_buffer, out_buffer);
    }
    test_utils::assert_buffer_value(0.0f, out_buffer);
}

TEST_F(TestSimpleSynthPlugin, TestNoteOnandOffSameCallback)
{
    ChunkSampleBuffer in_buffer(TEST_CHANNEL_COUNT);
    ChunkSampleBuffer out_buffer(TEST_CHANNEL_COUNT);

    RtEvent note_on = RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f);
    _module_under_test->process_event(note_on);
    RtEvent note_off = RtEvent::make_note_off_event(0, 1, 0, 60, 1.0f);
    _module_under_test->process_event(note_off);
    note_on = RtEvent::make_note_on_event(0, 2, 0, 60, 1.0f);
    _module_under_test->process_event(note_on);

    _module_under_test->process_event(note_on);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_non_null(out_buffer);
}

TEST_F(TestSimpleSynthPlugin, TestNoNaNsUnderStress)
{
    // just go crazy with NoteONs + parameter changes and verify that no NaNs are generated

    ChunkSampleBuffer in_buffer(TEST_CHANNEL_COUNT);
    ChunkSampleBuffer out_buffer(TEST_CHANNEL_COUNT);

    std::ranlux24 rand_dev;
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    std::uniform_int_distribution<int> note_dist{0, 127};


    for (int i = 0; i < 128; i++)
    {
        for (auto& pd : _module_under_test->all_parameters())
        {
            _module_under_test->process_event(RtEvent::make_parameter_change_event(_module_under_test->id(),
                                                                                   0,
                                                                                   pd->id(),
                                                                                   dist(rand_dev)));
        }
        RtEvent note_on = RtEvent::make_note_on_event(0, 0, 0, note_dist(rand_dev), 1.0f);
        _module_under_test->process_event(note_on);

        _module_under_test->process_audio(in_buffer, out_buffer);
        test_utils::assert_buffer_not_nan(out_buffer);
    }
}

