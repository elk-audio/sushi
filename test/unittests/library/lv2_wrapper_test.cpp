#include "gtest/gtest.h"

#define private public

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"

#include "test_utils/engine_mockup.h"
#include "library/lv2_features.cpp"
#include "library/lv2_wrapper.cpp"
#include "library/lv2_model.cpp"
#include "library/lv2_control.cpp"

using namespace sushi;
using namespace sushi::lv2;

// Reference output signal from JX10 LV2 plugin
// in response to NoteON C4 (60), vel=127, default parameters
static constexpr float LV2SYNTH_EXPECTED_OUT_NOTE_ON[2][64] = {
    {
        0.0f, -1.323203546732543e-09f, -5.813897627215958e-11f, 7.417479963578444e-09f, 1.688927042664545e-08f,
        2.093830175908806e-08f, 6.33123908855282e-09f, -4.370598460923247e-08f, -1.513699317001738e-07f,
        -3.422686916110251e-07f, -6.472507152466278e-07f, -1.100336021409021e-06f, -1.740671905281488e-06f,
        -2.610295041449717e-06f, -3.756287469514064e-06f, -5.228318968875101e-06f, -7.081040166667663e-06f,
        -9.371377018396743e-06f, -1.216118289448787e-05f, -1.551427158119623e-05f, -1.949933357536793e-05f,
        -2.418670555925928e-05f, -2.96515499940142e-05f, -3.597036629798822e-05f, -4.322444146964699e-05f,
        -5.149606658960693e-05f, -6.087229485274293e-05f, -7.144088158383965e-05f, -8.329428237630054e-05f,
        -9.652535663917661e-05f, -0.0001112316313083284f, -0.0001275107351830229f, -0.0001454649027436972f,
        -0.0001656185195315629f, -0.0001877363829407841f, -0.0002119304990628734f, -0.0002383175160503015f,
        -0.0002670133544597775f, -0.0002981385041493922f, -0.0003318124508950859f, -0.0003681592643260956f,
        -0.0004073016170877963f, -0.0004493667220231146f, -0.0004944801330566406f, -0.0005427719443105161f,
        -0.0005943701835349202f, -0.0006494075059890747f, -0.0007080142968334258f, -0.0007703251321800053f,
        -0.0008364724926650524f, -0.0009065927006304264f, -0.000980819808319211f, -0.001059291651472449f,
        -0.001142143737524748f, -0.001229515066370368f, -0.001321541960351169f, -0.001418364583514631f,
        -0.0015201197238639f, -0.001626948127523065f, -0.001738987164571881f, -0.001856377813965082f,
        -0.001979257678613067f, -0.002107767388224602f, -0.00224204477854073f
    },
    {
        0.0f, -1.323203546732543e-09f, -5.813897627215958e-11f, 7.417479963578444e-09f, 1.688927042664545e-08f,
        2.093830175908806e-08f, 6.33123908855282e-09f, -4.370598460923247e-08f, -1.513699317001738e-07f,
        -3.422686916110251e-07f, -6.472507152466278e-07f, -1.100336021409021e-06f, -1.740671905281488e-06f,
        -2.610295041449717e-06f, -3.756287469514064e-06f, -5.228318968875101e-06f, -7.081040166667663e-06f,
        -9.371377018396743e-06f, -1.216118289448787e-05f, -1.551427158119623e-05f, -1.949933357536793e-05f,
        -2.418670555925928e-05f, -2.96515499940142e-05f, -3.597036629798822e-05f, -4.322444146964699e-05f,
        -5.149606658960693e-05f, -6.087229485274293e-05f, -7.144088158383965e-05f, -8.329428237630054e-05f,
        -9.652535663917661e-05f, -0.0001112316313083284f, -0.0001275107351830229f, -0.0001454649027436972f,
        -0.0001656185195315629f, -0.0001877363829407841f, -0.0002119304990628734f, -0.0002383175160503015f,
        -0.0002670133544597775f, -0.0002981385041493922f, -0.0003318124508950859f, -0.0003681592643260956f,
        -0.0004073016170877963f, -0.0004493667220231146f, -0.0004944801330566406f, -0.0005427719443105161f,
        -0.0005943701835349202f, -0.0006494075059890747f, -0.0007080142968334258f, -0.0007703251321800053f,
        -0.0008364724926650524f, -0.0009065927006304264f, -0.000980819808319211f, -0.001059291651472449f,
        -0.001142143737524748f, -0.001229515066370368f, -0.001321541960351169f, -0.001418364583514631f,
        -0.0015201197238639f, -0.001626948127523065f, -0.001738987164571881f, -0.001856377813965082f,
        -0.001979257678613067f, -0.002107767388224602f, -0.00224204477854073f
    }
};

static constexpr float LV2SYNTH_EXPECTED_OUT_NOTE_OFF[2][64] = {
    {
        -0.002116627991199493f, -0.001996544189751148f, -0.001884167781099677f, -0.001776634831912816f,
        -0.001673887250944972f, -0.001575847039930522f, -0.001482423394918442f, -0.001393510843627155f,
        -0.00130899460054934f, -0.001228750217705965f, -0.001152646960690618f, -0.001080547808669508f,
        -0.001012312714010477f, -0.0009477980784140527f, -0.0008868593722581863f, -0.0008293509599752724f,
        -0.0007751278462819755f, -0.0007240457343868911f, -0.0006759626558050513f, -0.0006307382136583328f,
        -0.0005882354453206062f, -0.0005483202403411269f, -0.0005108620971441269f, -0.0004757342685479671f,
        -0.0004428141401149333f, -0.0004119832010474056f, -0.0003831273352261633f, -0.0003561366465874016f,
        -0.0003309058665763587f, -0.0003073339466936886f, -0.0002853243495337665f, -0.0002647849032655358f,
        -0.0002456277143210173f, -0.0002277691091876477f, -0.0002111295907525346f, -0.0001957935455720872f,
        -0.000181502357008867f, -0.0001681915018707514f, -0.0001557998330099508f, -0.0001442693755961955f,
        -0.0001335452980129048f, -0.000123575737234205f, -0.0001143117624451406f, -0.0001057071785908192f,
        -9.771844634087756e-05f, -9.030457295011729e-05f, -8.342699584318325e-05f, -7.704945892328396e-05f,
        -7.113791798474267e-05f, -6.566040974576026e-05f, -6.05869572609663e-05f, -5.588946805801243e-05f,
        -5.154165410203859e-05f, -4.751888627652079e-05f, -4.379814708954655e-05f, -4.035792153445072e-05f,
        -3.717811705428176e-05f, -3.423996167839505e-05f, -3.15259640046861e-05f, -2.90197986032581e-05f,
        -2.6706265998655e-05f, 0.0f, 0.0f, 0.0f
    },
    {
        -0.002116627991199493f, -0.001996544189751148f, -0.001884167781099677f, -0.001776634831912816f,
        -0.001673887250944972f, -0.001575847039930522f, -0.001482423394918442f, -0.001393510843627155f,
        -0.00130899460054934f, -0.001228750217705965f, -0.001152646960690618f, -0.001080547808669508f,
        -0.001012312714010477f, -0.0009477980784140527f, -0.0008868593722581863f, -0.0008293509599752724f,
        -0.0007751278462819755f, -0.0007240457343868911f, -0.0006759626558050513f, -0.0006307382136583328f,
        -0.0005882354453206062f, -0.0005483202403411269f, -0.0005108620971441269f, -0.0004757342685479671f,
        -0.0004428141401149333f, -0.0004119832010474056f, -0.0003831273352261633f, -0.0003561366465874016f,
        -0.0003309058665763587f, -0.0003073339466936886f, -0.0002853243495337665f, -0.0002647849032655358f,
        -0.0002456277143210173f, -0.0002277691091876477f, -0.0002111295907525346f, -0.0001957935455720872f,
        -0.000181502357008867f, -0.0001681915018707514f, -0.0001557998330099508f, -0.0001442693755961955f,
        -0.0001335452980129048f, -0.000123575737234205f, -0.0001143117624451406f, -0.0001057071785908192f,
        -9.771844634087756e-05f, -9.030457295011729e-05f, -8.342699584318325e-05f, -7.704945892328396e-05f,
        -7.113791798474267e-05f, -6.566040974576026e-05f, -6.05869572609663e-05f, -5.588946805801243e-05f,
        -5.154165410203859e-05f, -4.751888627652079e-05f, -4.379814708954655e-05f, -4.035792153445072e-05f,
        -3.717811705428176e-05f, -3.423996167839505e-05f, -3.15259640046861e-05f, -2.90197986032581e-05f,
        -2.6706265998655e-05f, 0.0f, 0.0f, 0.0f
    }
};

constexpr float TEST_SAMPLE_RATE = 48000;

class TestLv2Wrapper : public ::testing::Test
{
protected:
    TestLv2Wrapper()
    {
    }

    void SetUp(const std::string& plugin_URI)
    {
        _module_under_test = new lv2::Lv2Wrapper(_host_control.make_host_control_mockup(TEST_SAMPLE_RATE), plugin_URI);

        auto ret = _module_under_test->init(TEST_SAMPLE_RATE);
        ASSERT_EQ(ProcessorReturnCode::OK, ret);
        _module_under_test->set_event_output(&_fifo);
        _module_under_test->set_enabled(true);
    }

    void TearDown()
    {
        delete _module_under_test;
    }

    RtEventFifo _fifo;

    HostControlMockup _host_control;
    Lv2Wrapper* _module_under_test{nullptr};
};

TEST_F(TestLv2Wrapper, TestSetName)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    EXPECT_EQ("http://lv2plug.in/plugins/eg-amp", _module_under_test->name());
    EXPECT_EQ("Simple Amplifier", _module_under_test->label());
}

TEST_F(TestLv2Wrapper, TestParameterInitialization)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    auto gain_param = _module_under_test->parameter_from_name("Gain");
    EXPECT_TRUE(gain_param);
    EXPECT_EQ(0u, gain_param->id());
}

TEST_F(TestLv2Wrapper, TestParameterSetViaEvent)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    auto event = RtEvent::make_parameter_change_event(0, 0, 0, 0.123f);
    _module_under_test->process_event(event);
    auto value = _module_under_test->parameter_value(0);
    EXPECT_EQ(0.123f, value.second);
}

TEST_F(TestLv2Wrapper, TestProcessing)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    ChunkSampleBuffer in_buffer(1);
    ChunkSampleBuffer out_buffer(1);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    _module_under_test->process_audio(in_buffer, out_buffer);

    test_utils::assert_buffer_value(1.0f, out_buffer);
}

TEST_F(TestLv2Wrapper, TestProcessingWithParameterChanges)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    ChunkSampleBuffer in_buffer(1);
    ChunkSampleBuffer out_buffer(1);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);

    // Verify that a parameter change affects the sound.
    // eg-amp plugins Gain parameter range is from -90 to 24
    auto event = RtEvent::make_parameter_change_event(0, 0, 0, -90.0f);
    _module_under_test->process_event(event);

    _module_under_test->process_audio(in_buffer, out_buffer);

    test_utils::assert_buffer_value(0.0f, out_buffer);

    auto [status, value] = _module_under_test->parameter_value(0);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ(-90.0f, value);
}

TEST_F(TestLv2Wrapper, TestBypassProcessing)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    ChunkSampleBuffer in_buffer(1);
    ChunkSampleBuffer out_buffer(1);
    auto event = RtEvent::make_parameter_change_event(0, 0, 0, -90.0f);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    _module_under_test->set_bypassed(true);
    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);
}

TEST_F(TestLv2Wrapper, TestMidiEventInput)
{
    SetUp("http://moddevices.com/plugins/mda/JX10");

    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);

    _module_under_test->process_event(RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f));

    _module_under_test->process_audio(in_buffer, out_buffer);

    for (int i=0; i<2; i++)
    {
        for (int j=0; j < std::min(AUDIO_CHUNK_SIZE, 64); j++)
        {
            ASSERT_FLOAT_EQ(LV2SYNTH_EXPECTED_OUT_NOTE_ON[i][j], out_buffer.channel(i)[j]);
        }
    }

    // The JX10 synthesizer doesn't seem to have the snappiest of release times.
    // For a buffer of 64 samples, only the very last few are 0,
    // even when setting the envelope release to 0.

    // Setting ENV_Rel to 0. It has parameter ID 18.
    auto event = RtEvent::make_parameter_change_event(0, 0, 18, 0.0f);
    _module_under_test->process_event(event);

    _module_under_test->process_event(RtEvent::make_note_off_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    for (int i=0; i<2; i++)
    {
        for (int j=0; j < std::min(AUDIO_CHUNK_SIZE, 64); j++)
        {
           ASSERT_FLOAT_EQ(LV2SYNTH_EXPECTED_OUT_NOTE_OFF[i][j], out_buffer.channel(i)[j]);
        }
    }
}

TEST_F(TestLv2Wrapper, TestMidiEventInputAndOutput)
{
    SetUp("http://lv2plug.in/plugins/eg-fifths");

    ASSERT_TRUE(_fifo.empty());

    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);

    _module_under_test->process_event(RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_event(RtEvent::make_note_off_event(0, 0, 0, 60, 0.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    RtEvent e;
    bool got_event = _fifo.pop(e);
    ASSERT_TRUE(got_event);

    ASSERT_EQ(_module_under_test->id(), e.processor_id());

    ASSERT_EQ(RtEventType::NOTE_ON, e.type());
    ASSERT_EQ(60, e.keyboard_event()->note());

    _fifo.pop(e);

    ASSERT_EQ(RtEventType::NOTE_ON, e.type());
    ASSERT_EQ(67, e.keyboard_event()->note());

    _fifo.pop(e);

    ASSERT_EQ(RtEventType::NOTE_OFF, e.type());
    ASSERT_EQ(60, e.keyboard_event()->note());

    _fifo.pop(e);

    ASSERT_EQ(RtEventType::NOTE_OFF, e.type());
    ASSERT_EQ(67, e.keyboard_event()->note());

    ASSERT_TRUE(_fifo.empty());
}

// TODO Ilias: Currently crashes, due to update speaker arrangement not being populated.
/*TEST_F(TestLv2Wrapper, TestMonoProcess)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");
    ChunkSampleBuffer mono_buffer(1);
    ChunkSampleBuffer stereo_buffer(2);

    _module_under_test->set_input_channels(1);
    EXPECT_TRUE(_module_under_test->_double_mono_input);
    test_utils::fill_sample_buffer(mono_buffer, 1.0f);
    _module_under_test->process_audio(mono_buffer, stereo_buffer);
    test_utils::assert_buffer_value(1.0f, stereo_buffer);

    _module_under_test->set_output_channels(1);
    _module_under_test->set_input_channels(2);
    test_utils::fill_sample_buffer(stereo_buffer, 2.0f);
    _module_under_test->process_audio(stereo_buffer, mono_buffer);
    test_utils::assert_buffer_value(2.0f, mono_buffer);
}*/

// TODO Ilias: Re-instate once time info is implemented.
/*TEST_F(TestLv2Wrapper, TestTimeInfo)
{
    SetUp("libagain.so");
    _host_control._transport.set_tempo(60);
    _host_control._transport.set_time_signature({4, 4});
    _host_control._transport.set_time(std::chrono::seconds(1), static_cast<int64_t>(TEST_SAMPLE_RATE));
    auto time_info = _module_under_test->time_info();
    EXPECT_EQ(static_cast<int64_t>(TEST_SAMPLE_RATE), time_info->samplePos);
    EXPECT_EQ(1'000'000'000, time_info->nanoSeconds);
    EXPECT_FLOAT_EQ(1.0f, time_info->ppqPos);
    EXPECT_FLOAT_EQ(60.0f, time_info->tempo);
    EXPECT_FLOAT_EQ(0.0f, time_info->barStartPos);
    EXPECT_EQ(4, time_info->timeSigNumerator);
    EXPECT_EQ(4, time_info->timeSigDenominator);
}*/

// TODO: Re-instate
/*TEST_F(TestLv2Wrapper, TestConfigurationChange)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    _module_under_test->configure(44100.0f);
    ASSERT_FLOAT_EQ(44100, _module_under_test->_sample_rate);
}*/

// TODO: Re-instate
/*TEST_F(TestLv2Wrapper, TestParameterChangeNotifications)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    ASSERT_FALSE(_host_control._dummy_dispatcher.got_event());
    _module_under_test->notify_parameter_change(0, 0.5f);
    auto event = std::move(_host_control._dummy_dispatcher.retrieve_event());
    ASSERT_FALSE(event == nullptr);
    ASSERT_TRUE(event->is_parameter_change_notification());
}*/

// TODO: Re-instate
/*TEST_F(TestLv2Wrapper, TestRTParameterChangeNotifications)
{
    SetUp("libagain.so");
    RtEventFifo queue;
    _module_under_test->set_event_output(&queue);
    ASSERT_TRUE(queue.empty());
    _module_under_test->notify_parameter_change_rt(0, 0.5f);
    RtEvent event;
    auto received = queue.pop(event);
    ASSERT_TRUE(received);
    ASSERT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
}*/

// TODO: Re-instate
/*TEST_F(TestLv2Wrapper, TestProgramManagement)
{
    SetUp("libvstxsynth.so");
    ASSERT_TRUE(_module_under_test->supports_programs());
    ASSERT_EQ(128, _module_under_test->program_count());
    ASSERT_EQ(0, _module_under_test->current_program());
    ASSERT_EQ("Basic", _module_under_test->current_program_name());
    auto [status, program_name] = _module_under_test->program_name(2);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    ASSERT_EQ("Basic", program_name);
    // Access with an invalid program number
    std::tie(status, program_name) = _module_under_test->program_name(2000);
    ASSERT_NE(ProcessorReturnCode::OK, status);
    // Get all programs, all programs are named "Basic" in VstXSynth
    auto [res, programs] = _module_under_test->all_program_names();
    ASSERT_EQ(ProcessorReturnCode::OK, res);
    ASSERT_EQ("Basic", programs[50]);
    ASSERT_EQ(128u, programs.size());
}*/