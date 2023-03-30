/*
* Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
*
* SUSHI is free software: you can redistribute it and/or modify it under the terms of
* the GNU Affero General Public License as published by the Free Software Foundation,
* either version 3 of the License, or (at your option) any later version.
*
* SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License along with
* SUSHI. If not, see http://www.gnu.org/licenses/
*/

#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#include "test_utils/test_utils.h"

#define private public
#define protected public

#include "engine/controller/real_time_controller.cpp"

// TODO: Make these mock-able too!
#include "audio_frontends/reactive_frontend.cpp"
#include "control_frontends/reactive_midi_frontend.cpp"

#include "test_utils/audio_frontend_mockup.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"

using namespace std::chrono_literals;

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::_;

using namespace sushi;
using namespace sushi::internal;

constexpr float TEST_SAMPLE_RATE = 44100;

class ReactiveControllerTestFrontend : public ::testing::Test
{
protected:
    ReactiveControllerTestFrontend() {}

    void SetUp()
    {
        _real_time_controller = std::make_unique<RealTimeController>(&_audio_frontend,
                                                                     &_midi_frontend,
                                                                     &_transport);
    }

    void TearDown() {}

    EngineMockup _mock_engine {TEST_SAMPLE_RATE};
    ReactiveFrontend _audio_frontend {&_mock_engine};

    midi_dispatcher::MidiDispatcher _midi_dispatcher {_mock_engine.event_dispatcher()};

    sushi::internal::midi_frontend::ReactiveMidiFrontend _midi_frontend {&_midi_dispatcher};

    RtEventFifo<10> _rt_event_output;
    Transport _transport {TEST_SAMPLE_RATE, &_rt_event_output};

    std::unique_ptr<RealTimeController> _real_time_controller;
};

TEST_F(ReactiveControllerTestFrontend, TestRtControllerAudioCalls)
{
    // TODO: This test can be improved by mocking also ReactiveFrontend,
    //  so we can use "expect call" on it.

    ASSERT_FALSE(_mock_engine.process_called);

    ChunkSampleBuffer in_buffer;
    ChunkSampleBuffer out_buffer;

    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    _real_time_controller->process_audio(in_buffer, out_buffer, 2, 1s);

    test_utils::assert_buffer_value(1.0f, out_buffer);

    ASSERT_TRUE(_mock_engine.process_called);
}

TEST_F(ReactiveControllerTestFrontend, TestRtControllerTransportCalls)
{
    // Tempo:
    float old_tempo = _real_time_controller->_tempo;
    float new_tempo = 124.5f;

    _real_time_controller->set_tempo(new_tempo);

    EXPECT_NE(old_tempo, new_tempo);
    EXPECT_FLOAT_EQ(_real_time_controller->_tempo, new_tempo);
    EXPECT_FLOAT_EQ(_real_time_controller->_transport->current_tempo(), new_tempo); // Since we don't gmock Transport.

    // Time Signature:
    auto old_time_signature = _real_time_controller->_time_signature;
    ext::TimeSignature new_time_signature {5, 8};
    auto new_internal_time_signature = controller_impl::to_internal(new_time_signature);

    _real_time_controller->set_time_signature(new_time_signature);

    EXPECT_NE(old_time_signature, new_internal_time_signature);
    EXPECT_EQ(_real_time_controller->_time_signature, new_internal_time_signature);
    EXPECT_EQ(_real_time_controller->_transport->time_signature(),
              new_internal_time_signature); // Since we don't gmock Transport.

    // Playing Mode:
    auto old_playing_mode = _real_time_controller->_playing_mode;
    ext::PlayingMode new_playing_mode = ext::PlayingMode::PLAYING;
    auto new_internal_playing_mode = controller_impl::to_internal(new_playing_mode);

    _real_time_controller->set_playing_mode(new_playing_mode);

    EXPECT_NE(old_playing_mode, new_playing_mode);
    EXPECT_EQ(_real_time_controller->_playing_mode, new_playing_mode);

    // TODO: Mock Transport.
    // Only on set_time is playing_mode updated in Transport.
    EXPECT_NE(_real_time_controller->_transport->playing_mode(), new_internal_playing_mode);
    _real_time_controller->_transport->set_time(std::chrono::seconds(0), 0);
    EXPECT_EQ(_real_time_controller->_transport->playing_mode(), new_internal_playing_mode);

    // Beat Count & Position Source (they interact):
    auto old_beat_count = _real_time_controller->_transport->_beat_count;
    double new_beat_count = 14.5;
    _real_time_controller->set_current_beats(new_beat_count);

    EXPECT_NE(new_beat_count, old_beat_count);
    EXPECT_NE(new_beat_count, _real_time_controller->_transport->_beat_count);

    _real_time_controller->set_position_source(TransportPositionSource::EXTERNAL);

    _real_time_controller->set_current_beats(new_beat_count);

    EXPECT_DOUBLE_EQ(new_beat_count, _real_time_controller->_transport->_beat_count);
}

TEST_F(ReactiveControllerTestFrontend, TestRtControllerMidiCalls)
{
    // TODO: Currently the Passive Controller MIDI handling over the Passive MIDI frontend, is unfinished,
    //   and not real-time safe. Once it's finished (story AUD-456), we should add relevant tests also here.
}
