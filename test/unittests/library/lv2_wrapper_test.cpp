#include "gtest/gtest.h"

#define private public

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"

#include "test_utils/engine_mockup.h"
#include "library/lv2/lv2_state.cpp"
#include "library/lv2/lv2_features.cpp"
#include "library/lv2/lv2_wrapper.cpp"
#include "library/lv2/lv2_port.cpp"
#include "library/lv2/lv2_model.cpp"

#include <iomanip>

using namespace sushi;
using namespace sushi::lv2;

static const float LV2_ORGAN_EXPECTED_OUT_NOTE_ON[2][64] = {
    {
        -1.9887361676e-02f, 9.0109853772e-04f, 2.0839706063e-02f, 3.8966707885e-02f,
        5.4478537291e-02f, 6.6768631339e-02f, 7.5469307601e-02f, 8.0477617681e-02f,
        8.1962041557e-02f, 8.0348283052e-02f, 7.6286055148e-02f, 7.0598401129e-02f,
        6.4218170941e-02f, 5.8116056025e-02f, 5.3226333112e-02f, 5.0374895334e-02f,
        5.0216168165e-02f, 5.3182411939e-02f, 5.9450294822e-02f, 6.8926192820e-02f,
        8.1251896918e-02f, 9.5830298960e-02f, 1.1186864227e-01f, 1.2843678892e-01f,
        1.4453560114e-01f, 1.5917070210e-01f, 1.7142613232e-01f, 1.8053224683e-01f,
        1.8592296541e-01f, 1.8727806211e-01f, 1.8454769254e-01f, 1.7795649171e-01f,
        1.6798810661e-01f, 1.5535037220e-01f, 1.4092400670e-01f, 1.2569887936e-01f,
        1.1070217937e-01f, 9.6924163401e-02f, 8.5246242583e-02f, 7.6378263533e-02f,
        7.0807389915e-02f, 6.8764515221e-02f, 7.0209108293e-02f, 7.4834592640e-02f,
        8.2092970610e-02f, 9.1237813234e-02f, 1.0138138384e-01f, 1.1156232655e-01f,
        1.2081874907e-01f, 1.2826091051e-01f, 1.3313876092e-01f, 1.3489858806e-01f,
        1.3322512805e-01f, 1.2806573510e-01f, 1.1963484436e-01f, 1.0839867592e-01f,
        9.5040790737e-02f, 8.0411612988e-02f, 6.5465413034e-02f, 5.1189281046e-02f,
        3.8529783487e-02f, 2.8322366998e-02f, 2.1228877828e-02f, 1.7687896267e-02f
    },
    {
        1.9887385890e-02f, 3.1153870746e-02f, 4.1664179415e-02f, 5.0943803042e-02f,
        5.8579005301e-02f, 6.4260825515e-02f, 6.7806936800e-02f, 6.9174423814e-02f,
        6.8462260067e-02f, 6.5903387964e-02f, 6.1847079545e-02f, 5.6732773781e-02f,
        5.1057439297e-02f, 4.5338720083e-02f, 4.0076959878e-02f, 3.5718422383e-02f,
        3.2623004168e-02f, 3.1038269401e-02f, 3.1082302332e-02f, 3.2736111432e-02f,
        3.5846497864e-02f, 4.0139075369e-02f, 4.5240271837e-02f, 5.0706971437e-02f,
        5.6061267853e-02f, 6.0827869922e-02f, 6.4571462572e-02f, 6.6930904984e-02f,
        6.7648090422e-02f, 6.6589057446e-02f, 6.3755989075e-02f, 5.9289049357e-02f,
        5.3458150476e-02f, 4.6645279974e-02f, 3.9318550378e-02f, 3.2000318170e-02f,
        2.5231275707e-02f, 1.9533682615e-02f, 1.5375670977e-02f, 1.3139910065e-02f,
        1.3097739778e-02f, 1.5391272493e-02f, 2.0023778081e-02f, 2.6859069243e-02f,
        3.5629525781e-02f, 4.5952223241e-02f, 5.7351749390e-02f, 6.9288566709e-02f,
        8.1191159785e-02f, 9.2490255833e-02f, 1.0265340656e-01f, 1.1121802777e-01f,
        1.1782121658e-01f, 1.2222464383e-01f, 1.2433256954e-01f, 1.2420214713e-01f,
        1.2204400450e-01f, 1.1821295321e-01f, 1.1318840832e-01f, 1.0754481703e-01f,
        1.0191382468e-01f, 9.6939742565e-02f, 9.3231752515e-02f, 9.1315768659e-02f
    }
};

static const float LV2_ORGAN_EXPECTED_OUT_NOTE_OFF[2][64] = {
    {
        1.7881659791e-02f, 2.1721476689e-02f, 2.8853684664e-02f, 3.8684703410e-02f,
        5.0424069166e-02f, 6.3141867518e-02f, 7.5836457312e-02f, 8.7507508695e-02f,
        9.7228780389e-02f, 1.0421558470e-01f, 1.0788147897e-01f, 1.0788055509e-01f,
        1.0413187742e-01f, 9.6824221313e-02f, 8.6400978267e-02f, 7.3526442051e-02f,
        5.9035707265e-02f, 4.3872114271e-02f, 2.9016954824e-02f, 1.5416422859e-02f,
        3.9113699459e-03f, -4.8247324303e-03f, -1.0335511528e-02f, -1.2414798141e-02f,
        -1.1121401563e-02f, -6.7735435441e-03f, 7.5477364589e-05f, 8.6797121912e-03f,
        1.8157035112e-02f, 2.7556037530e-02f, 3.5927888006e-02f, 4.2398385704e-02f,
        4.6234156936e-02f, 4.6898778528e-02f, 4.4094085693e-02f, 3.7783939391e-02f,
        2.8198553249e-02f, 1.5819150954e-02f, 1.3437479502e-03f, -1.4362812042e-02f,
        -3.0331481248e-02f, -4.5559264719e-02f, -5.9082139283e-02f, -7.0045031607e-02f,
        -7.7763967216e-02f, -8.1775456667e-02f, -8.1869937479e-02f, -7.8106127679e-02f,
        -7.0806145668e-02f, -6.0530900955e-02f, -4.8038166016e-02f, -3.4226257354e-02f,
        -2.0067496225e-02f, -6.5364628099e-03f, 5.4617957212e-03f, 1.5157972462e-02f,
        2.1974716336e-02f, 2.5568073615e-02f, 2.5851685554e-02f, 2.3001641035e-02f,
        1.7441695556e-02f, 9.8100304604e-03f, 9.1005861759e-04f, -8.3511536941e-03f
    },
    {
        9.1590367258e-02f, 9.4289809465e-02f, 9.9458612502e-02f, 1.0693941265e-01f,
        1.1637654155e-01f, 1.2723566592e-01f, 1.3883808255e-01f, 1.5040819347e-01f,
        1.6113007069e-01f, 1.7020900548e-01f, 1.7693307996e-01f, 1.8072973192e-01f,
        1.8121278286e-01f, 1.7821559310e-01f, 1.7180767655e-01f, 1.6229356825e-01f,
        1.5019322932e-01f, 1.3620585203e-01f, 1.2115979195e-01f, 1.0595214367e-01f,
        9.1482840478e-02f, 7.8588657081e-02f, 6.7981503904e-02f, 6.0196351260e-02f,
        5.5552419275e-02f, 5.4130889475e-02f, 5.5770248175e-02f, 6.0080390424e-02f,
        6.6473536193e-02f, 7.4210308492e-02f, 8.2456864417e-02f, 9.0349331498e-02f,
        9.7060017288e-02f, 1.0186109692e-01f, 1.0418038815e-01f, 1.0364528000e-01f,
        1.0011153668e-01f, 9.3674950302e-02f, 8.4664218128e-02f, 7.3616817594e-02f,
        6.1238475144e-02f, 4.8350155354e-02f, 3.5825949162e-02f, 2.4527007714e-02f,
        1.5236089006e-02f, 8.5978982970e-03f, 5.0696432590e-03f, 4.8854770139e-03f,
        8.0376062542e-03f, 1.4275408350e-02f, 2.3122791201e-02f, 3.3912342042e-02f,
        4.5833855867e-02f, 5.7993568480e-02f, 6.9479763508e-02f, 7.9429633915e-02f,
        8.7092712522e-02f, 9.1885775328e-02f, 9.3435429037e-02f, 9.1605030000e-02f,
        8.6503803730e-02f, 7.8477859497e-02f, 6.8083383143e-02f, 5.6044187397e-02f
    }
};

static const float LV2_ORGAN_EXPECTED_OUT_AFTER_PROGRAM_CHANGE[2][64] = {
    {
        -4.0559459478e-02f, -1.2670414150e-01f, -1.6014342010e-01f, -1.3931660354e-01f,
        -1.0104077309e-01f, -4.3121892959e-02f, -2.6207653806e-02f, -2.3199958727e-02f,
        -6.7270337604e-03f, -7.1769892238e-03f, 1.6412626952e-02f, 6.5241619945e-02f,
        7.2027243674e-02f, 4.1552502662e-02f, 4.9767918885e-02f, 5.0462573767e-02f,
        9.7279725596e-03f, 1.8221000209e-02f, 3.5446345806e-02f, 9.9497539923e-03f,
        1.7544374568e-03f, 1.5812514350e-02f, 1.9179783762e-02f, 9.7139133140e-03f,
        9.6436059102e-03f, 9.7721070051e-03f, -2.9934497434e-04f, -1.2593343854e-03f,
        -4.9335660879e-04f, 2.5154934265e-03f, 1.3092795387e-02f, 2.3186910897e-02f,
        3.2880228013e-02f, 3.7889908999e-02f, 3.6271773279e-02f, 3.0448844656e-02f,
        2.4073433131e-02f, 2.2794701159e-02f, 2.9039207846e-02f, 3.9371751249e-02f,
        4.4091433287e-02f, 3.1205622479e-02f, -5.2744448185e-03f, -5.7064604014e-02f,
        -1.0211383551e-01f, -1.1757943779e-01f, -1.0098508745e-01f, -7.8470312059e-02f,
        -8.2318276167e-02f, -1.0812826455e-01f, -1.1286567897e-01f, -8.7147615850e-02f,
        -8.7160207331e-02f, -9.9829167128e-02f, -4.9523893744e-02f, 7.3941829614e-03f,
        2.1289000288e-02f, 3.4531883895e-02f, 2.9814595357e-02f, 2.8686856851e-02f,
        6.4323723316e-02f, 8.3953127265e-02f, 5.7771116495e-02f, 2.5712912902e-02f
    },
    {
        -4.1594504728e-04f, 6.0177445412e-03f, 3.5308238119e-02f, 7.0668801665e-02f,
        6.8747535348e-02f, 4.5336712152e-02f, 4.7007802874e-02f, 5.2566956729e-02f,
        3.1826283783e-02f, 1.9984606653e-02f, 2.7318730950e-02f, 2.4265950546e-02f,
        2.4262389168e-02f, 2.4067681283e-02f, 8.3705848083e-03f, 1.5788340941e-02f,
        4.2923163623e-02f, 3.1943585724e-02f, -1.8917510286e-02f, -5.1602017134e-02f,
        -4.9048338085e-02f, -4.1848506778e-02f, -2.5306237862e-02f, -1.3802642003e-02f,
        -3.4754935652e-02f, -4.7641962767e-02f, -3.3736333251e-02f, -3.5233095288e-02f,
        -5.7496655732e-02f, -4.4349476695e-02f, -9.2654824257e-03f, -6.3623310998e-03f,
        -8.7197721004e-03f, -1.5063190833e-02f, -2.4213692173e-02f, -9.8235614132e-04f,
        2.4544438347e-02f, 1.6821755096e-02f, -6.3254060224e-03f, -1.1017292272e-03f,
        1.0261813179e-02f, -9.6444161609e-03f, -9.0680299327e-03f, 6.1845094897e-03f,
        -2.9426216497e-04f, -3.1293779612e-03f, -1.2182801962e-02f, -2.7193233371e-02f,
        -2.7497438714e-02f, -2.2942436859e-02f, -2.0740259439e-02f, -2.2964352742e-02f,
        -1.8179310486e-02f, -8.2391826436e-03f, -1.4105647802e-02f, -2.1977031603e-02f,
        -2.0145220682e-02f, -1.9941106439e-02f, -1.7214599997e-02f, -1.7495656386e-02f,
        -2.8904944658e-02f, -3.7753723562e-02f, -3.6063570529e-02f, -3.3155035228e-02f
    }
};

// Utility for creating the above static buffers by copying values from console.
void print_buffer(ChunkSampleBuffer& buffer, int channels)
{
    std::cout << std::scientific << std::setprecision(10);
    int print_i = 0;
    for (int i = 0; i < channels; i++)
    {
        for (int j = 0; j < std::min(AUDIO_CHUNK_SIZE, 64); j++)
        {
            std::cout << buffer.channel(i)[j] << "f, ";
            print_i++;

            if(print_i == 4)
            {
                std::cout << std::endl;
                print_i = 0;
            }
        }

        std::cout << std::endl;
    }
}

void compare_buffers(const float static_array[][64], ChunkSampleBuffer& buffer, int channels)
{
    for (int i = 0; i < channels; i++)
    {
        for (int j = 0; j < std::min(AUDIO_CHUNK_SIZE, 64); j++)
        {
            ASSERT_FLOAT_EQ(static_array[i][j], buffer.channel(i)[j]);
        }
    }
}

constexpr float TEST_SAMPLE_RATE = 48000;

class TestLv2Wrapper : public ::testing::Test
{
protected:
    TestLv2Wrapper()
    {
    }

    void SetUp(const std::string& plugin_URI)
    {
        _module_under_test = std::make_unique<lv2::Lv2Wrapper>(_host_control.make_host_control_mockup(TEST_SAMPLE_RATE), plugin_URI);

        auto ret = _module_under_test->init(TEST_SAMPLE_RATE);

        if (ret == ProcessorReturnCode::SHARED_LIBRARY_OPENING_ERROR)
        {
            _module_under_test = nullptr;
            return;
        }

        ASSERT_EQ(ProcessorReturnCode::OK, ret);
        _module_under_test->set_event_output(&_fifo);
        _module_under_test->set_enabled(true);
    }

    void TearDown()
    {

    }

    RtSafeRtEventFifo _fifo;

    HostControlMockup _host_control;
    std::unique_ptr<Lv2Wrapper> _module_under_test {nullptr};
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

/*
 * Depends on the CALF Organ plugin, which uses Fluidsynth internally.
 * Since this is relatively heavy to load, several tests are done in one method.
 * 1. Basic program management calls
 * 2. Audio check after note on
 * 3. Audio check after note off
 * 4. Different audio after program change message.
 *
 * If the Calf plugin is not found, the test just returns after printing a message to the console.
 */
TEST_F(TestLv2Wrapper, TestOrgan)
{
    SetUp("http://calf.sourceforge.net/plugins/Organ");

    if (_module_under_test == nullptr)
    {
        std::cout << "Calf Organ plugin not installed - please install it to ensure full suite of unit tests has run." << std::endl;
        return;
    }

    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);

    ASSERT_TRUE(_module_under_test->supports_programs());
    ASSERT_EQ(29, _module_under_test->program_count());
    ASSERT_EQ(0, _module_under_test->current_program());
    ASSERT_EQ("http://calf.sourceforge.net/factory_presets#organ_12Sqr", _module_under_test->current_program_name());
    auto [status, program_name] = _module_under_test->program_name(2);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    ASSERT_EQ("http://calf.sourceforge.net/factory_presets#organ_CriticalBass", program_name);

    // Access with an invalid program number
    std::tie(status, program_name) = _module_under_test->program_name(2000);
    ASSERT_NE(ProcessorReturnCode::OK, status);

    // Get all programs, all programs are named "Basic" in VstXSynth
    auto [res, programs] = _module_under_test->all_program_names();
    ASSERT_EQ(ProcessorReturnCode::OK, res);
    ASSERT_EQ("http://calf.sourceforge.net/factory_presets#organ_RoyalewithCheese", programs[15]);
    ASSERT_EQ(29u, programs.size());

    _module_under_test->process_event(RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    compare_buffers(LV2_ORGAN_EXPECTED_OUT_NOTE_ON, out_buffer, 2);

    _module_under_test->process_event(RtEvent::make_note_off_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    compare_buffers(LV2_ORGAN_EXPECTED_OUT_NOTE_OFF, out_buffer, 2);

    // A compromise, for the unit tests to be able to run, while still having a sempaphore in the live multithreaded program.
    _module_under_test->_pause();

    _module_under_test->set_program(1);

    // A compromise, for the unit tests to be able to run, while still having a sempaphore in the live multithreaded program.
    _module_under_test->_resume();

    _module_under_test->process_event(RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);

    compare_buffers(LV2_ORGAN_EXPECTED_OUT_AFTER_PROGRAM_CHANGE, out_buffer, 2);

    _module_under_test->process_event(RtEvent::make_note_off_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);
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

TEST_F(TestLv2Wrapper, TestConfigurationChange)
{
    SetUp("http://lv2plug.in/plugins/eg-amp");

    _module_under_test->configure(44100.0f);
    ASSERT_FLOAT_EQ(44100, _module_under_test->_sample_rate);
}

// TODO: Re-instate once time info is implemented.
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