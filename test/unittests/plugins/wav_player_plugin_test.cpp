#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"
#include "library/rt_event_fifo.h"

#define private public

#include "plugins/wav_player_plugin.cpp"

using namespace sushi;
using namespace sushi::wav_player_plugin;

constexpr float TEST_SAMPLERATE = 44100;
const float SAMPLE_DATA[] = {1.0f, 2.0f, 2.0f, 1.0f, 1.0f};
const int SAMPLE_DATA_LENGTH = sizeof(SAMPLE_DATA) / sizeof(float);

static const std::string SAMPLE_FILE = "Kawai-K11-GrPiano-C4_mono.wav";


/* Test the Voice class */
class TestWaveStreamerPlugin : public ::testing::Test
{
protected:
    void SetUp()
    {
    }

};
