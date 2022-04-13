#include <fstream>
#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"
#include "engine/json_configurator.h"
#include "test_utils/test_utils.h"

#define private public
#include "audio_frontends/offline_frontend.cpp"

using ::testing::internal::posix::GetEnv;

using namespace sushi;
using namespace sushi::audio_frontend;
using namespace sushi::midi_dispatcher;

constexpr float SAMPLE_RATE = 44000;
constexpr int CV_CHANNELS = 0;
constexpr int AUDIO_CHANNELS = 2;

class TestOfflineFrontend : public ::testing::Test
{
protected:
    TestOfflineFrontend()
    {
    }

    void SetUp()
    {
        _module_under_test = new OfflineFrontend(&_engine);
        _engine.set_audio_input_channels(AUDIO_CHANNELS);
        _engine.set_audio_output_channels(AUDIO_CHANNELS);
    }

    void TearDown()
    {
        delete _module_under_test;
    }

    EngineMockup _engine{SAMPLE_RATE};
    MidiDispatcher _midi_dispatcher{_engine.event_dispatcher()};
    OfflineFrontend* _module_under_test;
};

TEST_F(TestOfflineFrontend, TestWavProcessing)
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable";
    }

    // Initialize with a file containing 0.5 on both channels
    std::string test_data_file(test_data_dir);
    test_data_file.append("/test_sndfile_05.wav");
    std::string output_file_name("./test_out.wav");
    OfflineFrontendConfiguration config(test_data_file, "./test_out.wav", false, CV_CHANNELS, CV_CHANNELS);
    auto ret_code = _module_under_test->init(&config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        EXPECT_TRUE(false) << "Error initializing Frontend";
    }
    // Process with the dummy bypass engine
    _module_under_test->run();

    // Read the generated file and verify the result
    SNDFILE*    output_file;
    SF_INFO     soundfile_info;
    memset(&soundfile_info, 0, sizeof(soundfile_info));

    if (! (output_file = sf_open(output_file_name.c_str(), SFM_READ, &soundfile_info)) )
    {
        EXPECT_TRUE(false) << "Error opening output file: " << output_file_name;
    }

    float file_buffer[AUDIO_CHANNELS * AUDIO_CHUNK_SIZE];
    unsigned int readcount;
    while ( (readcount = static_cast<unsigned int>(sf_readf_float(output_file,
                                                         &file_buffer[0],
                                                         static_cast<sf_count_t>(AUDIO_CHUNK_SIZE)))) )
    {
        for (unsigned int n=0; n<(readcount * AUDIO_CHANNELS); n++)
        {
            ASSERT_FLOAT_EQ(0.5f, file_buffer[n]);
        }
    }

    sf_close(output_file);
}

TEST_F(TestOfflineFrontend, TestInvalidInputFile)
{
    OfflineFrontendConfiguration config("this_is_not_a_valid_file.extension", "./test_out.wav", false, CV_CHANNELS, CV_CHANNELS);
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::INVALID_INPUT_FILE, ret_code);
}

TEST_F(TestOfflineFrontend, TestMonoMode)
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable";
    }
    // Initialize with a mono file
    std::string test_data_file(test_data_dir);
    test_data_file.append("/mono.wav");
    std::string output_file_name("./test_out.wav");
    OfflineFrontendConfiguration config(test_data_file, "./test_out.wav", false, CV_CHANNELS, CV_CHANNELS);
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, ret_code);

    // Process with the dummy bypass engine and make check this doesn't crash
    _module_under_test->run();
}

TEST_F(TestOfflineFrontend, TestAddSequencerEvents)
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable\n";
    }

    // Initialize with a file containing 0.5 on both channels
    std::string test_config_file = test_utils::get_data_dir_path();
    test_config_file.append("config.json");
    sushi::jsonconfig::JsonConfigurator configurator(&_engine,
                                                     &_midi_dispatcher,
                                                     _engine.processor_container(),
                                                     test_config_file);
    rapidjson::Document config;
    auto [status, events] = configurator.load_event_list();
    ASSERT_EQ(jsonconfig::JsonConfigReturnStatus::OK, status);
    _module_under_test->add_sequencer_events(events);

    auto event_q = _module_under_test->_event_queue;
    ASSERT_EQ(4u, event_q.size());

    // Check that queue is sorted by time
    auto jt = --event_q.end();
    for(auto it = event_q.begin(); it != jt; ++it)
    {
        ASSERT_GE((*it)->time(), (*(it + 1))->time());
    }

    // Clear events manually, as that would be done by the EventDispatcher otherwise
    for (auto* event : events)
    {
        delete event;
    }

}

TEST_F(TestOfflineFrontend, TestNoiseGeneration)
{
    ChunkSampleBuffer buffer(2);
    std::ranlux24 rand_gen;
    rand_gen.seed(NOISE_SEED);
    std::normal_distribution<float> normal_dist(0, INPUT_NOISE_LEVEL);
    fill_buffer_with_noise(buffer, rand_gen, normal_dist);

    float mean = 0.0f;
    for (int c = 0; c < buffer.channel_count(); ++c)
    {
        for (float* s = buffer.channel(c); s < buffer.channel(c) + AUDIO_CHUNK_SIZE; ++s)
        {
            mean += *s * *s;
        }
    }
    mean /= AUDIO_CHUNK_SIZE * buffer.channel_count();
    float rms = std::sqrt(mean);
    ASSERT_NEAR(INPUT_NOISE_LEVEL, rms, 0.002f);
}

TEST(TestAudioFrontendInternals, TestRampCvOutput)
{
    float data_buffer[AUDIO_CHUNK_SIZE];
    auto target = ramp_cv_output(data_buffer, 1, 0.5);
    EXPECT_EQ(0.5f, target);
    float prev = 1.1f;
    for (auto i : data_buffer)
    {
        EXPECT_GT(prev, i);
        prev = i;
    }
}