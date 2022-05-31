#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#define private public
#define protected public

#include "engine/controller/passive_controller.cpp"

// TODO: Make these mock-able too!
#include "audio_frontends/passive_frontend.cpp"
#include "control_frontends/passive_midi_frontend.cpp"

#include "test_utils/audio_frontend_mockup.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"
#include "test_utils/mock_sushi.h"

using namespace std::chrono_literals;

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::_;

using namespace sushi;

constexpr float TEST_SAMPLE_RATE = 44100;

class PassiveControllerTestFrontend : public ::testing::Test
{
protected:
    PassiveControllerTestFrontend() {}

    void SetUp()
    {
        _passive_controller = std::make_unique<PassiveController>(std::make_unique<MockSushi>());
        _mock_sushi = static_cast<MockSushi*>(_passive_controller->_sushi.get());

        // These test initialization.
        // Since Init is needed throughout the below I nonetheless keep them here,
        // to avoid repetition in the code.
        SushiOptions options;

        EXPECT_CALL(*_mock_sushi, init)
            .Times(1)
            .WillRepeatedly(Return(InitStatus::OK));

        EXPECT_CALL(*_mock_sushi, audio_frontend)
            .Times(1)
            .WillRepeatedly(Return(&_audio_frontend));

        EXPECT_CALL(*_mock_sushi, midi_frontend)
            .Times(1);

        EXPECT_CALL(*_mock_sushi, audio_engine)
            .Times(1)
            .WillOnce(Return(&_mock_engine));

        EXPECT_CALL(*_mock_sushi, start)
            .Times(1);

        auto status = _passive_controller->init(options);

        EXPECT_EQ(InitStatus::OK, status);
    }

    void TearDown()
    {
        EXPECT_CALL(*_mock_sushi, exit)
            .Times(1);
    }

    std::unique_ptr<PassiveController> _passive_controller;

    MockSushi* _mock_sushi;

    EngineMockup _mock_engine {TEST_SAMPLE_RATE};
    PassiveFrontend _audio_frontend {&_mock_engine};
};

TEST_F(PassiveControllerTestFrontend, TestSushiOwnerAccessors)
{
    EXPECT_CALL(*_mock_sushi, set_sample_rate)
        .Times(1);

    _passive_controller->set_sample_rate(TEST_SAMPLE_RATE);

    EXPECT_FLOAT_EQ(TEST_SAMPLE_RATE, _passive_controller->sample_rate());
}

TEST_F(PassiveControllerTestFrontend, TestRtControllerAudioCalls)
{
    // TODO: This can be improved by mocking also PassiveFrontend, so we can use "expect call" on it.
    _passive_controller->process_audio(2, 1s);
    EXPECT_TRUE(&_passive_controller->in_buffer() == &_audio_frontend.in_buffer());
    EXPECT_TRUE(&_passive_controller->out_buffer() == &_audio_frontend.out_buffer());
}

TEST_F(PassiveControllerTestFrontend, TestRtControllerTransportCalls)
{
    // Tempo:
    float old_tempo = _passive_controller->_tempo;
    float new_tempo = 124.5f;

    _passive_controller->set_tempo(new_tempo);

    EXPECT_NE(old_tempo, new_tempo);
    EXPECT_FLOAT_EQ(_passive_controller->_tempo, new_tempo);
    EXPECT_FLOAT_EQ(_passive_controller->_transport->current_tempo(), new_tempo); // Since we don't gmock Transport.

    // Time Signature:
    auto old_time_signature = _passive_controller->_time_signature;
    ext::TimeSignature new_time_signature {5, 8};
    auto new_internal_time_signature = engine::controller_impl::to_internal(new_time_signature);

    _passive_controller->set_time_signature(new_time_signature);

    EXPECT_NE(old_time_signature, new_internal_time_signature);
    EXPECT_EQ(_passive_controller->_time_signature, new_internal_time_signature);
    EXPECT_EQ(_passive_controller->_transport->time_signature(), new_internal_time_signature); // Since we don't gmock Transport.

    // Playing Mode:
    auto old_playing_mode = _passive_controller->_playing_mode;
    ext::PlayingMode new_playing_mode = ext::PlayingMode::PLAYING;
    auto new_internal_playing_mode = engine::controller_impl::to_internal(new_playing_mode);

    _passive_controller->set_playing_mode(new_playing_mode);

    EXPECT_NE(old_playing_mode, new_playing_mode);
    EXPECT_EQ(_passive_controller->_playing_mode, new_playing_mode);

    // TODO: Mock Transport.
    // Only on set_time is playing_mode updated in Transport.
    EXPECT_NE(_passive_controller->_transport->playing_mode(), new_internal_playing_mode);
    _passive_controller->_transport->set_time(std::chrono::seconds(0), 0);
    EXPECT_EQ(_passive_controller->_transport->playing_mode(), new_internal_playing_mode);

    // Beat Count & Position Source (they interact):
    auto old_beat_count = _passive_controller->_transport->_beat_count;
    double new_beat_count = 14.5;
    _passive_controller->set_current_beats(new_beat_count);

    EXPECT_NE(new_beat_count, old_beat_count);
    EXPECT_NE(new_beat_count, _passive_controller->_transport->_beat_count);

    _passive_controller->set_position_source(TransportPositionSource::EXTERNAL);

    _passive_controller->set_current_beats(new_beat_count);

    EXPECT_DOUBLE_EQ(new_beat_count, _passive_controller->_transport->_beat_count);
}

TEST_F(PassiveControllerTestFrontend, TestRtControllerMidiCalls)
{
    // TODO: Currently the Passive Controller MIDI handling over the Passive MIDI frontend, is unfinished,
    //   and not real-time safe. Once it's finished (story AUD-456), we should add relevant tests also here.
}

