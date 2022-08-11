#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"

#include "test_utils/engine_mockup.h"
#include "library/lv2/lv2_state.cpp"
#include "library/lv2/lv2_features.cpp"

// Needed for unit tests to access private utility methods in lv2_wrapper.
#define private public

#include "library/lv2/lv2_wrapper.cpp"

#include "library/lv2/lv2_port.cpp"
#include "library/lv2/lv2_model.cpp"
#include "library/lv2/lv2_worker.cpp"
#include "library/lv2/lv2_control.cpp"

using namespace sushi;
using namespace sushi::lv2;

constexpr float TEST_SAMPLE_RATE = 48000;
constexpr int   TEST_CHANNEL_COUNT = 2;

// Tip: use 'test_utils::print_buffer<64>(out_buffer, 1)'
// to generate static buffer content in text like the below.
static const float LV2_SAMPLER_EXPECTED_OUT_NOTE_ON[1][64] = {
    {
        8.5811443627e-02f, 1.5257082880e-01f, 1.9981867075e-01f, 2.6550742984e-01f,
        3.1894043088e-01f, 3.6774355173e-01f, 4.2173218727e-01f, 4.7939392924e-01f,
        5.2299284935e-01f, 5.6986409426e-01f, 6.1811023951e-01f, 6.6184180975e-01f,
        7.0369291306e-01f, 7.4014079571e-01f, 7.8293782473e-01f, 8.1396549940e-01f,
        8.4733998775e-01f, 8.7596982718e-01f, 8.9703381062e-01f, 9.2847150564e-01f,
        9.4153606892e-01f, 9.6092289686e-01f, 9.7848141193e-01f, 9.7645491362e-01f,
        9.9383544922e-01f, 9.9207293987e-01f, 9.8401486874e-01f, 9.9248045683e-01f,
        9.8064988852e-01f, 9.7262036800e-01f, 9.5535176992e-01f, 9.4097930193e-01f,
        9.2831599712e-01f, 8.9614868164e-01f, 8.7789064646e-01f, 8.4527707100e-01f,
        8.1868839264e-01f, 7.8956091404e-01f, 7.3800379038e-01f, 7.0913290977e-01f,
        6.6314095259e-01f, 6.1984896660e-01f, 5.8228880167e-01f, 5.2198934555e-01f,
        4.8422691226e-01f, 4.2829886079e-01f, 3.6879178882e-01f, 3.2674816251e-01f,
        2.6642197371e-01f, 2.1852211654e-01f, 1.5994016826e-01f, 1.0220003873e-01f,
        4.7169614583e-02f, -2.0063044503e-02f, -7.0752970874e-02f, -1.3219092786e-01f,
        -1.8827511370e-01f, -2.4446520209e-01f, -3.0550417304e-01f, -3.5686719418e-01f,
        -4.1207060218e-01f, -4.5643404126e-01f, -5.0429958105e-01f, -5.5731260777e-01f
    }
};

static const float LV2_SAMPLER_EXPECTED_OUT_NOTE_OFF[1][64] = {
    {
        -6.0585808754e-01f, -6.5102791786e-01f, -6.9954341650e-01f, -7.3845398426e-01f,
        -7.6650136709e-01f, -8.0487918854e-01f, -8.4129923582e-01f, -8.7295645475e-01f,
        -8.9587861300e-01f, -9.3141978979e-01f, -9.5426052809e-01f, -9.5808660984e-01f,
        -9.8191249371e-01f, -9.9042803049e-01f, -9.9751955271e-01f, -1.0013926029e+00f,
        -9.9870741367e-01f, -1.0012304783e+00f, -9.9909341335e-01f, -9.9926203489e-01f,
        -9.7940754890e-01f, -9.6884649992e-01f, -9.5232754946e-01f, -9.2335337400e-01f,
        -9.1003942490e-01f, -8.7584549189e-01f, -8.5085171461e-01f, -8.1899070740e-01f,
        -7.7134633064e-01f, -7.4802970886e-01f, -7.0600986481e-01f, -6.6105115414e-01f,
        -6.2150335312e-01f, -5.6417763233e-01f, -5.2068632841e-01f, -4.6517598629e-01f,
        -4.1045567393e-01f, -3.6629143357e-01f, -3.0408981442e-01f, -2.5660526752e-01f,
        -1.9908088446e-01f, -1.4596894383e-01f, -9.3292877078e-02f, -2.3014366627e-02f,
        2.4260010570e-02f, 8.2992948592e-02f, 1.4165890217e-01f, 1.9666172564e-01f,
        2.5683587790e-01f, 3.1207546592e-01f, 3.6573141813e-01f, 4.1807574034e-01f,
        4.7608512640e-01f, 5.1777309179e-01f, 5.6559085846e-01f, 6.1442857981e-01f,
        6.5522098541e-01f, 7.0046389103e-01f, 7.4100756645e-01f, 7.7862900496e-01f,
        8.1257081032e-01f, 8.4414005280e-01f, 8.7060534954e-01f, 8.9672446251e-01f
    }
};

class TestLv2Wrapper : public ::testing::Test
{
protected:
    using ::testing::Test::SetUp; // Hide error of hidden overload of virtual function in clang when signatures differ but the name is the same
    TestLv2Wrapper()
    {
    }

    ProcessorReturnCode SetUp(const std::string& plugin_URI)
    {
        auto mockup = _host_control.make_host_control_mockup(TEST_SAMPLE_RATE);
        _world = std::make_shared<LilvWorldWrapper>();
        bool world_created = _world->create_world();
        EXPECT_TRUE(world_created);
        _module_under_test = std::make_unique<lv2::LV2_Wrapper>(mockup, plugin_URI, _world);

        auto ret = _module_under_test->init(TEST_SAMPLE_RATE);

        if (ret != ProcessorReturnCode::OK)
        {
            _module_under_test = nullptr;
        }
        else
        {
            _module_under_test->set_event_output(&_fifo);
            _module_under_test->set_enabled(true);
            _module_under_test->set_input_channels(std::min(TEST_CHANNEL_COUNT, _module_under_test->max_input_channels()));
            _module_under_test->set_output_channels(std::min(TEST_CHANNEL_COUNT, _module_under_test->max_output_channels()));
        }

        return ret;
    }

    void TearDown()
    {
        _module_under_test = nullptr;
    }

    RtSafeRtEventFifo _fifo;

    HostControlMockup _host_control;
    std::shared_ptr<LilvWorldWrapper> _world;
    std::unique_ptr<LV2_Wrapper> _module_under_test;
};

TEST_F(TestLv2Wrapper, TestLV2PluginInvalidURI)
{
    // Closing console error output temporarily.
    // Lilv complains the URI is invalid, as it should.
    // We don't need that to pollute our unit test output.
    fclose(stderr);

    auto ret = SetUp("This URI surely does not exist.");

    // And re-opening the output:
    [[maybe_unused]] auto unused = freopen("/dev/tty", "w", stderr);

    ASSERT_EQ(ProcessorReturnCode::SHARED_LIBRARY_OPENING_ERROR, ret);
    ASSERT_EQ(_module_under_test, nullptr);
}

TEST_F(TestLv2Wrapper, TestLV2PluginInteraction)
{
    auto ret = SetUp("http://lv2plug.in/plugins/eg-amp");
    ASSERT_EQ(ProcessorReturnCode::OK, ret);

    int pCount = _module_under_test->parameter_count();
    EXPECT_EQ(1, pCount);
    auto param0 = _module_under_test->parameter_from_id(0);
    EXPECT_TRUE(param0);
    EXPECT_EQ(0u, param0->id());

    auto paramNull = _module_under_test->parameter_from_id(1);
    EXPECT_EQ(paramNull, nullptr);

    // TestSetName
    EXPECT_EQ("http://lv2plug.in/plugins/eg-amp", _module_under_test->name());
    EXPECT_EQ("Simple Amplifier", _module_under_test->label());

    // TestParameterInitialization
    auto gain_param = _module_under_test->parameter_from_name("Gain");
    EXPECT_TRUE(gain_param);
    EXPECT_EQ(0u, gain_param->id());

    // TestParameterSetViaEvent
    auto parameter_change_event = RtEvent::make_parameter_change_event(0, 0, 0, 0.5f);
    _module_under_test->process_event(parameter_change_event);
    auto value = _module_under_test->parameter_value(0);
    EXPECT_EQ(0.5f, value.second);

    // TestFetchingFormattedParameterValue
    auto [status, formattedValue] = _module_under_test->parameter_value_formatted(0);
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ("-33.000000", formattedValue);
}

TEST_F(TestLv2Wrapper, TestProcessingWithParameterChanges)
{
    auto ret = SetUp("http://lv2plug.in/plugins/eg-amp");
    ASSERT_EQ(ProcessorReturnCode::OK, ret);

    ChunkSampleBuffer in_buffer(1);
    ChunkSampleBuffer out_buffer(1);

    // TestProcessingWithParameterChanges
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);

    // Verify that a parameter change affects the sound.
    // eg-amp plugins Gain parameter range is from -90 to 24
    auto lower_gain_Event = RtEvent::make_parameter_change_event(0, 0, 0, 0.0f);
    _module_under_test->process_event(lower_gain_Event);

    _module_under_test->process_audio(in_buffer, out_buffer);

    test_utils::assert_buffer_value(0.0f, out_buffer);

    auto [status, parameter_value] = _module_under_test->parameter_value(0);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ(0.0f, parameter_value);
}

TEST_F(TestLv2Wrapper, TestBypassProcessing)
{
    auto ret = SetUp("http://lv2plug.in/plugins/eg-amp");
    ASSERT_EQ(ProcessorReturnCode::OK, ret);

    ChunkSampleBuffer in_buffer(1);
    ChunkSampleBuffer out_buffer(1);
    auto event = RtEvent::make_parameter_change_event(0, 0, 0, 0.3f);
    _module_under_test->process_event(event);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    // Set bypass and manually feed the generated RtEvent back to the
    // wrapper processor as event dispatcher is not running
    _module_under_test->set_bypassed(true);
    auto bypass_event = _host_control._dummy_dispatcher.retrieve_event();
    EXPECT_TRUE(bypass_event.get());

    _module_under_test->process_event(bypass_event->to_rt_event(0));
    EXPECT_TRUE(_module_under_test->bypassed());

    _module_under_test->process_audio(in_buffer, out_buffer);
    // Test that we are ramping up the audio to the bypass value
    float prev_value = 0;
    for (int i = 1; i < AUDIO_CHUNK_SIZE; ++i)
    {
        EXPECT_GT(out_buffer.channel(0)[i], prev_value);
        prev_value = out_buffer.channel(0)[i];
    }
}

TEST_F(TestLv2Wrapper, TestMidiEventInputAndOutput)
{
    auto ret = SetUp("http://lv2plug.in/plugins/eg-fifths");
    ASSERT_EQ(ProcessorReturnCode::OK, ret);

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

/* TODO: Currently incomplete.
 * Complete this with fetching of transport info from LV2 plugin, to compare states.
 * As it is now, at least it demonstrates that an LV2 plugin that requires the 'time' extension,
 * successfully loads.
*/
TEST_F(TestLv2Wrapper, TestTimeInfo)
{
    auto ret = SetUp("http://lv2plug.in/plugins/eg-metro");
    ASSERT_EQ(ProcessorReturnCode::OK, ret);

    _host_control._transport.set_tempo(60, false);
    _host_control._transport.set_time_signature({4, 4}, false);
    _host_control._transport.set_time(std::chrono::seconds(1), static_cast<int64_t>(TEST_SAMPLE_RATE));

    // Currently, The Sushi LV2 hosting does not get time info from plugin - it only sets.
    // So we cannot directly replicate the below.
    /*
    auto time_info = _module_under_test->time_info();
    EXPECT_EQ(static_cast<int64_t>(TEST_SAMPLE_RATE), time_info->samplePos);
    EXPECT_EQ(1'000'000'000, time_info->nanoSeconds);
    EXPECT_FLOAT_EQ(1.0f, time_info->ppqPos);
    EXPECT_FLOAT_EQ(60.0f, time_info->tempo);
    EXPECT_FLOAT_EQ(0.0f, time_info->barStartPos);
    EXPECT_EQ(4, time_info->timeSigNumerator);
    EXPECT_EQ(4, time_info->timeSigDenominator);
     */
}

// TODO: This tests synchronous worker invocation.
// Asynchronous invocation requires an additional LV2 extension to be implemented first,
// optional for eg-sampler, but required for its sample-loading feature.
TEST_F(TestLv2Wrapper, TestSynchronousStateAndWorkerThreads)
{
    auto ret = SetUp("http://lv2plug.in/plugins/eg-sampler");
    ASSERT_EQ(ProcessorReturnCode::OK, ret);

    ChunkSampleBuffer in_buffer(1);
    ChunkSampleBuffer out_buffer(1);

    _module_under_test->process_event(RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    // Increment channels to 2 when supporting stereo loading of mono plugins.
    test_utils::compare_buffers(LV2_SAMPLER_EXPECTED_OUT_NOTE_ON, out_buffer, 1, 0.0001f);

    _module_under_test->process_event(RtEvent::make_note_off_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    if (AUDIO_CHUNK_SIZE == 64)
    {
        // Increment channels to 2 when supporting stereo loading of mono plugins.
        test_utils::compare_buffers(LV2_SAMPLER_EXPECTED_OUT_NOTE_OFF, out_buffer, 1, 0.0001f);
    }
    else
    {
        std::cout << "AUDIO_CHUNK_SIZE != 64 - audio buffer comparisons after NOTE_OFF events in LV2 tests cannot run"
                  << std::endl;
    }
}

#ifdef SUSHI_BUILD_WITH_LV2_MDA_TESTS

static const float LV2_JX10_EXPECTED_OUT_NOTE_ON[2][64] = {
    {
        0.0000000000e+00f, -1.3231920004e-09f, -5.8071242259e-11f, 7.4176806919e-09f,
        1.6889693200e-08f, 2.0939033618e-08f, 6.3323604138e-09f, -4.3704385888e-08f,
        -1.5136777165e-07f, -3.4226587786e-07f, -6.4724713411e-07f, -1.1003317013e-06f,
        -1.7406666757e-06f, -2.6102886750e-06f, -3.7562799662e-06f, -5.2283103287e-06f,
        -7.0810297075e-06f, -9.3713651950e-06f, -1.2161169252e-05f, -1.5514257029e-05f,
        -1.9499317204e-05f, -2.4186683731e-05f, -2.9651528166e-05f, -3.5970344470e-05f,
        -4.3224412366e-05f, -5.1496041124e-05f, -6.0872265749e-05f, -7.1440852480e-05f,
        -8.3294245997e-05f, -9.6525320259e-05f, -1.1123159493e-04f, -1.2751069153e-04f,
        -1.4546485909e-04f, -1.6561846132e-04f, -1.8773633929e-04f, -2.1193045541e-04f,
        -2.3831747239e-04f, -2.6701329625e-04f, -2.9813844594e-04f, -3.3181239269e-04f,
        -3.6815920612e-04f, -4.0730155888e-04f, -4.4936660561e-04f, -4.9448001664e-04f,
        -5.4277182790e-04f, -5.9437012533e-04f, -6.4940738957e-04f, -7.0801418042e-04f,
        -7.7032501576e-04f, -8.3647237625e-04f, -9.0659258422e-04f, -9.8081969190e-04f,
        -1.0592915351e-03f, -1.1421436211e-03f, -1.2295149500e-03f, -1.3215418439e-03f,
        -1.4183644671e-03f, -1.5201196074e-03f, -1.6269480111e-03f, -1.7389869317e-03f,
        -1.8563776975e-03f, -1.9792574458e-03f, -2.1077671554e-03f, -2.2420443129e-03f
    },
    {
        0.0000000000e+00f, -1.3231920004e-09f, -5.8071242259e-11f, 7.4176806919e-09f,
        1.6889693200e-08f, 2.0939033618e-08f, 6.3323604138e-09f, -4.3704385888e-08f,
        -1.5136777165e-07f, -3.4226587786e-07f, -6.4724713411e-07f, -1.1003317013e-06f,
        -1.7406666757e-06f, -2.6102886750e-06f, -3.7562799662e-06f, -5.2283103287e-06f,
        -7.0810297075e-06f, -9.3713651950e-06f, -1.2161169252e-05f, -1.5514257029e-05f,
        -1.9499317204e-05f, -2.4186683731e-05f, -2.9651528166e-05f, -3.5970344470e-05f,
        -4.3224412366e-05f, -5.1496041124e-05f, -6.0872265749e-05f, -7.1440852480e-05f,
        -8.3294245997e-05f, -9.6525320259e-05f, -1.1123159493e-04f, -1.2751069153e-04f,
        -1.4546485909e-04f, -1.6561846132e-04f, -1.8773633929e-04f, -2.1193045541e-04f,
        -2.3831747239e-04f, -2.6701329625e-04f, -2.9813844594e-04f, -3.3181239269e-04f,
        -3.6815920612e-04f, -4.0730155888e-04f, -4.4936660561e-04f, -4.9448001664e-04f,
        -5.4277182790e-04f, -5.9437012533e-04f, -6.4940738957e-04f, -7.0801418042e-04f,
        -7.7032501576e-04f, -8.3647237625e-04f, -9.0659258422e-04f, -9.8081969190e-04f,
        -1.0592915351e-03f, -1.1421436211e-03f, -1.2295149500e-03f, -1.3215418439e-03f,
        -1.4183644671e-03f, -1.5201196074e-03f, -1.6269480111e-03f, -1.7389869317e-03f,
        -1.8563776975e-03f, -1.9792574458e-03f, -2.1077671554e-03f, -2.2420443129e-03f
    }
};

static const float LV2_JX10_EXPECTED_OUT_NOTE_OFF[2][64] = {
    {
        -2.3517450318e-03f, -2.4647361133e-03f, -2.5843831245e-03f, -2.7075796388e-03f,
        -2.8343601152e-03f, -2.9647541232e-03f, -3.0987935606e-03f, -3.2365065999e-03f,
        -3.3779235091e-03f, -3.5230703652e-03f, -3.6719755735e-03f, -3.8246638142e-03f,
        -3.9811609313e-03f, -4.1414904408e-03f, -4.3056765571e-03f, -4.4737402350e-03f,
        -4.6457038261e-03f, -4.8215868883e-03f, -5.0014094450e-03f, -5.1851901226e-03f,
        -5.3729466163e-03f, -5.5646947585e-03f, -5.7604517788e-03f, -5.9602307156e-03f,
        -6.1640464701e-03f, -6.3719111495e-03f, -6.5838382579e-03f, -6.7998361774e-03f,
        -7.0199179463e-03f, -7.2440896183e-03f, -7.4723623693e-03f, -7.7047408558e-03f,
        -7.9412339255e-03f, -8.1818439066e-03f, -8.4265777841e-03f, -8.6825294420e-03f,
        -8.9428499341e-03f, -9.2075373977e-03f, -9.4765927643e-03f, -9.7500113770e-03f,
        -1.0027793236e-02f, -1.0309931822e-02f, -1.0596422479e-02f, -1.0887259617e-02f,
        -1.1182436720e-02f, -1.1481943540e-02f, -1.1785773560e-02f, -1.2093913741e-02f,
        -1.2406354770e-02f, -1.2723082677e-02f, -1.3044086285e-02f, -1.3369349763e-02f,
        -1.3698859140e-02f, -1.4032597654e-02f, -1.4370549470e-02f, -1.4712693170e-02f,
        -1.5059012920e-02f, -1.5409486368e-02f, -1.5764094889e-02f, -1.6122814268e-02f,
        -1.6485624015e-02f, -1.6852496192e-02f, -1.7223412171e-02f, -1.7598342150e-02f
    },
    {
        -2.3517450318e-03f, -2.4647361133e-03f, -2.5843831245e-03f, -2.7075796388e-03f,
        -2.8343601152e-03f, -2.9647541232e-03f, -3.0987935606e-03f, -3.2365065999e-03f,
        -3.3779235091e-03f, -3.5230703652e-03f, -3.6719755735e-03f, -3.8246638142e-03f,
        -3.9811609313e-03f, -4.1414904408e-03f, -4.3056765571e-03f, -4.4737402350e-03f,
        -4.6457038261e-03f, -4.8215868883e-03f, -5.0014094450e-03f, -5.1851901226e-03f,
        -5.3729466163e-03f, -5.5646947585e-03f, -5.7604517788e-03f, -5.9602307156e-03f,
        -6.1640464701e-03f, -6.3719111495e-03f, -6.5838382579e-03f, -6.7998361774e-03f,
        -7.0199179463e-03f, -7.2440896183e-03f, -7.4723623693e-03f, -7.7047408558e-03f,
        -7.9412339255e-03f, -8.1818439066e-03f, -8.4265777841e-03f, -8.6825294420e-03f,
        -8.9428499341e-03f, -9.2075373977e-03f, -9.4765927643e-03f, -9.7500113770e-03f,
        -1.0027793236e-02f, -1.0309931822e-02f, -1.0596422479e-02f, -1.0887259617e-02f,
        -1.1182436720e-02f, -1.1481943540e-02f, -1.1785773560e-02f, -1.2093913741e-02f,
        -1.2406354770e-02f, -1.2723082677e-02f, -1.3044086285e-02f, -1.3369349763e-02f,
        -1.3698859140e-02f, -1.4032597654e-02f, -1.4370549470e-02f, -1.4712693170e-02f,
        -1.5059012920e-02f, -1.5409486368e-02f, -1.5764094889e-02f, -1.6122814268e-02f,
        -1.6485624015e-02f, -1.6852496192e-02f, -1.7223412171e-02f, -1.7598342150e-02f
    }
};

static const float LV2_JX10_EXPECTED_OUT_AFTER_PROGRAM_CHANGE[2][64] = {
    {
        -1.8251772970e-02f, -1.8858999014e-02f, -1.9479092211e-02f, -2.0112285390e-02f,
        -2.0495397970e-02f, -2.0881604403e-02f, -2.1270930767e-02f, -2.1663406864e-02f,
        -2.2059064358e-02f, -2.2457933053e-02f, -2.2860042751e-02f, -2.3265430704e-02f,
        -2.3674124852e-02f, -2.4086162448e-02f, -2.4501578882e-02f, -2.4920403957e-02f,
        -2.5342678651e-02f, -2.5768432766e-02f, -2.6197709143e-02f, -2.6630543172e-02f,
        -2.7066973969e-02f, -2.7507038787e-02f, -2.7950776741e-02f, -2.8398228809e-02f,
        -2.8849432245e-02f, -2.9304428026e-02f, -2.9763258994e-02f, -3.0225966126e-02f,
        -3.0692586675e-02f, -3.1163167208e-02f, -3.1637746841e-02f, -3.2116372138e-02f,
        -3.2599080354e-02f, -3.3085912466e-02f, -3.3576924354e-02f, -3.4072149545e-02f,
        -3.4571636468e-02f, -3.5082843155e-02f, -3.5598631948e-02f, -3.6119010299e-02f,
        -3.6644104868e-02f, -3.7173725665e-02f, -3.7703011185e-02f, -3.8227867335e-02f,
        -3.8748357445e-02f, -3.9264310151e-02f, -3.9775639772e-02f, -4.0282223374e-02f,
        -4.0783967823e-02f, -4.1280753911e-02f, -4.1772484779e-02f, -4.2259056121e-02f,
        -4.2740367353e-02f, -4.3216321617e-02f, -4.3686818331e-02f, -4.4151764363e-02f,
        -4.4611062855e-02f, -4.5064624399e-02f, -4.5512352139e-02f, -4.5954164118e-02f,
        -4.6389967203e-02f, -4.6819675714e-02f, -4.7243207693e-02f, -4.7660473734e-02f
    },
    {
        -1.8251772970e-02f, -1.8858999014e-02f, -1.9479092211e-02f, -2.0112285390e-02f,
        -2.0495397970e-02f, -2.0881604403e-02f, -2.1270930767e-02f, -2.1663406864e-02f,
        -2.2059064358e-02f, -2.2457933053e-02f, -2.2860042751e-02f, -2.3265430704e-02f,
        -2.3674124852e-02f, -2.4086162448e-02f, -2.4501578882e-02f, -2.4920403957e-02f,
        -2.5342678651e-02f, -2.5768432766e-02f, -2.6197709143e-02f, -2.6630543172e-02f,
        -2.7066973969e-02f, -2.7507038787e-02f, -2.7950776741e-02f, -2.8398228809e-02f,
        -2.8849432245e-02f, -2.9304428026e-02f, -2.9763258994e-02f, -3.0225966126e-02f,
        -3.0692586675e-02f, -3.1163167208e-02f, -3.1637746841e-02f, -3.2116372138e-02f,
        -3.2599080354e-02f, -3.3085912466e-02f, -3.3576924354e-02f, -3.4072149545e-02f,
        -3.4571636468e-02f, -3.5082843155e-02f, -3.5598631948e-02f, -3.6119010299e-02f,
        -3.6644104868e-02f, -3.7173725665e-02f, -3.7703011185e-02f, -3.8227867335e-02f,
        -3.8748357445e-02f, -3.9264310151e-02f, -3.9775639772e-02f, -4.0282223374e-02f,
        -4.0783967823e-02f, -4.1280753911e-02f, -4.1772484779e-02f, -4.2259056121e-02f,
        -4.2740367353e-02f, -4.3216321617e-02f, -4.3686818331e-02f, -4.4151764363e-02f,
        -4.4611062855e-02f, -4.5064624399e-02f, -4.5512352139e-02f, -4.5954164118e-02f,
        -4.6389967203e-02f, -4.6819675714e-02f, -4.7243207693e-02f, -4.7660473734e-02f
    }
};

/*
 * Depends on the MDA JX10 Synth plugin, as ported by drobilla (there are more ports).
 * Since this is relatively heavy to load, several tests are done in one method.
 * 1. Basic program management calls
 * 2. Audio check after note on
 * 3. Audio check after note off
 * 4. Different audio after program change message.
 *
 * If the plugin is not found, the test just returns after printing a message to the console.
 */
TEST_F(TestLv2Wrapper, TestSynth)
{
    SetUp("http://drobilla.net/plugins/mda/JX10");

    ASSERT_TRUE(_module_under_test != nullptr)  << "'http://drobilla.net/plugins/mda/JX10' plugin not installed - please install it to ensure full suite of unit tests has run.";

    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);

    ASSERT_TRUE(_module_under_test->supports_programs());
    ASSERT_EQ(52, _module_under_test->program_count());
    ASSERT_EQ(0, _module_under_test->current_program());
    ASSERT_EQ("http://drobilla.net/plugins/mda/presets#JX10-303-saw-bass", _module_under_test->current_program_name());
    auto[status, program_name] = _module_under_test->program_name(2);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    ASSERT_EQ("http://drobilla.net/plugins/mda/presets#JX10-5th-sweep-pad", program_name);

    // Access with an invalid program number
    std::tie(status, program_name) = _module_under_test->program_name(2000);
    ASSERT_NE(ProcessorReturnCode::OK, status);

    // Get all programs, all programs are named "Basic" in VstXSynth
    auto[res, programs] = _module_under_test->all_program_names();
    ASSERT_EQ(ProcessorReturnCode::OK, res);
    ASSERT_EQ("http://drobilla.net/plugins/mda/presets#JX10-fretless-bass", programs[15]);
    ASSERT_EQ(52u, programs.size());

    _module_under_test->process_event(RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    test_utils::compare_buffers(LV2_JX10_EXPECTED_OUT_NOTE_ON, out_buffer, 2, 0.0001f);

    _module_under_test->process_event(RtEvent::make_note_off_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    if(AUDIO_CHUNK_SIZE == 64)
    {
        // Buffer comparisons after NOTE_OFF events in LV2 tests require buffer size == 64
        test_utils::compare_buffers(LV2_JX10_EXPECTED_OUT_NOTE_OFF, out_buffer, 2, 0.0001f);
    }

    // Setting program once first without checking audio output,
    // to ensure a sequence of changes goes through, not just the first one.
    _module_under_test->_pause_audio_processing();
    _module_under_test->set_program(5);
    _module_under_test->_resume_audio_processing();

    // A compromise, for the unit tests to be able to run.
    // It simulates the series of events in the live multithreaded program.
    _module_under_test->_pause_audio_processing();

    _module_under_test->set_program(1);

    // A compromise, for the unit tests to be able to run.
    // It simulates the series of events in the live multithreaded program.
    _module_under_test->_resume_audio_processing();

    _module_under_test->process_event(RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    if(AUDIO_CHUNK_SIZE == 64)
    {
        test_utils::compare_buffers(LV2_JX10_EXPECTED_OUT_AFTER_PROGRAM_CHANGE, out_buffer, 2,0.0001f);
    }

    _module_under_test->process_event(RtEvent::make_note_off_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);
}

TEST_F(TestLv2Wrapper, TestStateHandling)
{
    auto ret = SetUp("http://lv2plug.in/plugins/eg-amp");
    ASSERT_EQ(ProcessorReturnCode::OK, ret);

    auto desc = _module_under_test->parameter_from_name("Gain");
    ASSERT_TRUE(desc);

    ProcessorState state;
    state.set_bypass(true);
    state.set_program(2);
    state.add_parameter_change(desc->id(), 0.25);

    auto status = _module_under_test->set_state(&state, false);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    // Check that new values are set and update notification is queued
    EXPECT_FLOAT_EQ(0.25f, _module_under_test->parameter_value(desc->id()).second);
    EXPECT_TRUE(_module_under_test->bypassed());
    auto event = _host_control._dummy_dispatcher.retrieve_event();
    ASSERT_TRUE(event->is_engine_notification());

    // Test with realtime set to true
    state.set_bypass(false);
    state.set_program(1);
    state.add_parameter_change(desc->id(), 0.50);

    status = _module_under_test->set_state(&state, true);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    event = _host_control._dummy_dispatcher.retrieve_event();
    ASSERT_TRUE(event.get());
    _module_under_test->process_event(event->to_rt_event(0));

    // Check that new values are set
    EXPECT_FLOAT_EQ(0.5f, _module_under_test->parameter_value(desc->id()).second);
    EXPECT_FALSE(_module_under_test->bypassed());
}

TEST_F(TestLv2Wrapper, TestBinaryStateSaving)
{
    ChunkSampleBuffer buffer(2);

    auto ret = SetUp("http://lv2plug.in/plugins/eg-amp");
    ASSERT_EQ(ProcessorReturnCode::OK, ret);

    // Ugly hack to simulate audio not running.
    _module_under_test->_model->set_play_state(PlayState::PAUSED);

    auto desc = _module_under_test->parameter_from_name("Gain");
    ASSERT_TRUE(desc);
    float prev_value = _module_under_test->parameter_value(desc->id()).second;

    ProcessorState state = _module_under_test->save_state();
    ASSERT_TRUE(state.has_binary_data());

    // Set a parameter value, the re-apply the state
    auto rt_event = RtEvent::make_parameter_change_event(_module_under_test->id(), 0, desc->id(), 0.1234f);
    _module_under_test->process_event(rt_event);
    _module_under_test->process_audio(buffer, buffer);

    EXPECT_NE(prev_value, _module_under_test->parameter_value(desc->id()).second);

    auto status = _module_under_test->set_state(&state, false);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    // Check the value has reverted to the previous value
    EXPECT_FLOAT_EQ(prev_value, _module_under_test->parameter_value(desc->id()).second);
}

#endif //SUSHI_BUILD_WITH_LV2_MDA_TESTS
