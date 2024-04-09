/*
 * Copyright 2017-2023 Elk Audio AB
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

#include "elk-warning-suppressor/warning_suppressor.hpp"

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
#define protected public
ELK_POP_WARNING

#include "engine/controller/real_time_controller.cpp"

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
public:
    void crank_event_loop_once()
    {
        _event_dispatcher->_running = false;
        _event_dispatcher->_event_loop();
    }

protected:
    ReactiveControllerTestFrontend() = default;

    void SetUp() override
    {
        _mock_engine = std::make_unique<EngineMockup>(TEST_SAMPLE_RATE);

        _event_dispatcher = std::make_unique<dispatcher::EventDispatcher>(_mock_engine.get(), &_in_rt_queue, &_out_rt_queue);

        _audio_frontend = std::make_unique<ReactiveFrontend>(_mock_engine.get());

        _midi_dispatcher = std::make_unique<midi_dispatcher::MidiDispatcher>(_event_dispatcher.get());

        _midi_frontend = std::make_unique<midi_frontend::ReactiveMidiFrontend>(_midi_dispatcher.get());

        _real_time_controller = std::make_unique<RealTimeController>(_audio_frontend.get(),
                                                                     _midi_frontend.get(),
                                                                     &_transport);
    }

    std::unique_ptr<RealTimeController>          _real_time_controller;
    std::unique_ptr<dispatcher::EventDispatcher> _event_dispatcher;
    std::unique_ptr<EngineMockup>                _mock_engine;
    std::unique_ptr<ReactiveFrontend>            _audio_frontend;

    std::unique_ptr<midi_dispatcher::MidiDispatcher>     _midi_dispatcher;
    std::unique_ptr<midi_frontend::ReactiveMidiFrontend> _midi_frontend;

    RtSafeRtEventFifo _in_rt_queue;
    RtSafeRtEventFifo _out_rt_queue;

    Transport _transport {TEST_SAMPLE_RATE, &_out_rt_queue};
};

TEST_F(ReactiveControllerTestFrontend, TestRtControllerAudioCalls)
{
    ASSERT_FALSE(_mock_engine->process_called);

    ChunkSampleBuffer in_buffer;
    ChunkSampleBuffer out_buffer;

    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    _real_time_controller->process_audio(in_buffer, out_buffer, 1s);

    test_utils::assert_buffer_value(1.0f, out_buffer);

    ASSERT_TRUE(_mock_engine->process_called);
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
    control::TimeSignature new_time_signature {5, 8};
    auto new_internal_time_signature = controller_impl::to_internal(new_time_signature);

    _real_time_controller->set_time_signature(new_time_signature);

    EXPECT_NE(old_time_signature, new_internal_time_signature);
    EXPECT_EQ(_real_time_controller->_time_signature, new_internal_time_signature);
    EXPECT_EQ(_real_time_controller->_transport->time_signature(),
              new_internal_time_signature); // Since we don't gmock Transport.

    // Playing Mode:
    auto old_playing_mode = _real_time_controller->_playing_mode;
    control::PlayingMode new_playing_mode = control::PlayingMode::PLAYING;
    auto new_internal_playing_mode = controller_impl::to_internal(new_playing_mode);

    _real_time_controller->set_playing_mode(new_playing_mode);

    EXPECT_NE(old_playing_mode, new_playing_mode);
    EXPECT_EQ(_real_time_controller->_playing_mode, new_playing_mode);

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
    //  and not real-time safe. Once it's finished (story AUD-456), we should add relevant tests also here.
}
