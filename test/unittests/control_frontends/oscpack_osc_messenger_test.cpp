/*
* Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
*
* SUSHI is free software: you can redistribute it and/or modify it under the terms of
* the GNU Affero General Public License as published by the Free Software Foundation,
* either version 3 of the License, or (at your option) any later version.
*
* SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE.  See the GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License along with
* SUSHI.  If not, see http://www.gnu.org/licenses/
*/

#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock-actions.h>

#define private public
#define protected public

// This causes the real oscpack .h files for packet listener and socket,
// to be excluded in oscpack_osc_messenger.h
#define OSCPACK_UNIT_TESTS

#include "test_utils/mock_oscpack.h"
#include "test_utils/control_mockup.h"

#include "control_frontends/oscpack_osc_messenger.cpp"

namespace oscpack = ::osc;

#undef private
#undef protected

using ::testing::Return;
using ::testing::StrEq;
using ::testing::NiceMock;
using ::testing::_;

using namespace sushi;
using namespace sushi::control_frontend;
using namespace sushi::osc;

constexpr int OSC_TEST_SERVER_PORT = 24024;
constexpr int OSC_TEST_SEND_PORT = 24023;
constexpr auto OSC_TEST_SEND_ADDRESS = "127.0.0.1";

class TestOscpackOscMessenger : public ::testing::Test
{
protected:
    TestOscpackOscMessenger() {}

    void SetUp()
    {
        connection.processor = 0;
        connection.parameter = 0;
        connection.controller = &_mock_controller;

        _module_under_test = std::make_unique<OscpackOscMessenger>(OSC_TEST_SERVER_PORT,
                                                                   OSC_TEST_SEND_PORT,
                                                                   OSC_TEST_SEND_ADDRESS);

        _module_under_test->init();
    }

    void TearDown() {}

    sushi::ext::ControlMockup _mock_controller;

    OscConnection connection;

    IpEndpointName _endpoint{"", 0}; // Just to match ProcessMessage signature - the endpoint is unused.

    char _buffer[OSC_OUTPUT_BUFFER_SIZE];

    std::unique_ptr<OscpackOscMessenger> _module_under_test;
};

TEST_F(TestOscpackOscMessenger, TestAddAndRemoveConnections)
{
    EXPECT_EQ(_module_under_test->_last_generated_handle, 0);
    EXPECT_EQ(_module_under_test->_registered_messages.size(), 0);

    auto id_1 = _module_under_test->add_method("/engine/set_tempo",
                                              "f", sushi::osc::OscMethodType::SET_TEMPO,
                                              &connection);

    EXPECT_EQ(_module_under_test->_last_generated_handle, 1);
    EXPECT_EQ(_module_under_test->_registered_messages.size(), 1);
    EXPECT_EQ(reinterpret_cast<OSC_CALLBACK_HANDLE>(id_1), 0);

    // Attempting to register with an existing address_pattern and tts should fail:
    auto id_2 = _module_under_test->add_method("/engine/set_tempo",
                                               "f", sushi::osc::OscMethodType::SET_TEMPO,
                                               &connection);

    EXPECT_EQ(_module_under_test->_last_generated_handle, 1);
    EXPECT_EQ(_module_under_test->_registered_messages.size(), 1);
    EXPECT_EQ(reinterpret_cast<OSC_CALLBACK_HANDLE>(id_2), -1);

    // BUT the same AP with a different TTS, should be fine:
    auto id_3 = _module_under_test->add_method("/engine/set_tempo",
                                               "ff", sushi::osc::OscMethodType::SET_TEMPO,
                                               &connection);

    EXPECT_EQ(_module_under_test->_last_generated_handle, 2);
    EXPECT_EQ(_module_under_test->_registered_messages.size(), 2);
    EXPECT_EQ(reinterpret_cast<OSC_CALLBACK_HANDLE>(id_3), 1);

    // Calling with an unused ID should not remove anything:
    OSC_CALLBACK_HANDLE unused_id = 1234;
    _module_under_test->delete_method(reinterpret_cast<void*>(unused_id));
    EXPECT_EQ(_module_under_test->_registered_messages.size(), 2);

    // Calling delete_method with the returned ID should work:
    _module_under_test->delete_method(id_1);
    EXPECT_EQ(_module_under_test->_registered_messages.size(), 1);
}

TEST_F(TestOscpackOscMessenger, TestSendParameterChange)
{
    auto address_pattern = "/parameter/track_1/param_1";

    _module_under_test->add_method(address_pattern,
                                   "f", sushi::osc::OscMethodType::SEND_PARAMETER_CHANGE_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << 0.5f << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));

    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.parameter_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(0, std::stoi(args["parameter id"]));
    EXPECT_FLOAT_EQ(0.5f, std::stof(args["value"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/parameter/sampler/attack") << 5.0f << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSendPropertyChange)
{
    auto address_pattern = "/property/sampler/sample_file";

    _module_under_test->add_method(address_pattern,
                                   "s", sushi::osc::OscMethodType::SEND_PROPERTY_CHANGE_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "Sample file" << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));

    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.parameter_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(0, std::stoi(args["property id"]));
    EXPECT_EQ("Sample file", args["value"]);

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/property/sampler/attack") << 4 << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSendNoteOn)
{
    auto address_pattern = "/keyboard_event/sampler";

    _module_under_test->add_method(address_pattern,
                                   "siif", sushi::osc::OscMethodType::SEND_KEYBOARD_NOTE_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "note_on" << 0 << 46 << 0.8f << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));

    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(0, std::stoi(args["channel"]));
    EXPECT_EQ(46, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.8f, std::stof(args["velocity"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/keyboard_event/drums") << "note_on" << 4 << 20 << 0.2f << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSendNoteOff)
{
    auto address_pattern = "/keyboard_event/sampler";

    _module_under_test->add_method(address_pattern,
                                   "siif", sushi::osc::OscMethodType::SEND_KEYBOARD_NOTE_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "note_off" << 1 << 52 << 0.7f << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));

    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(1, std::stoi(args["channel"]));
    EXPECT_EQ(52, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.7f, std::stof(args["velocity"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/keyboard_event/drums") << "note_off" << 4 << 20 << 0.2f << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSendNoteAftertouch)
{
    auto address_pattern = "/keyboard_event/sampler";

    _module_under_test->add_method(address_pattern,
                                   "siif", sushi::osc::OscMethodType::SEND_KEYBOARD_NOTE_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "note_aftertouch" << 10 << 36 << 0.1f << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));

    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(10, std::stoi(args["channel"]));
    EXPECT_EQ(36, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.1f, std::stof(args["value"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/keyboard_event/drums") << "note_aftertouch" << 4 << 20 << 0.2f << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSendKeyboardModulation)
{
    auto address_pattern = "/keyboard_event/sampler";

    _module_under_test->add_method(address_pattern,
                                   "sif",
                                   sushi::osc::OscMethodType::SEND_KEYBOARD_MODULATION_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "modulation" << 9 << 0.5f << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));

    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(9, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.5f, std::stof(args["value"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/keyboard_event/drums") << "modulation" << 4 << 0.2f << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSendKeyboardPitchBend)
{
    auto address_pattern = "/keyboard_event/sampler";

    _module_under_test->add_method(address_pattern,
                                   "sif",
                                 sushi::osc::OscMethodType::SEND_KEYBOARD_MODULATION_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "pitch_bend" << 3 << 0.3f << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(3, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.3f, std::stof(args["value"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/keyboard_event/drums") << "pitch_bend" << 1 << 0.2f << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSendKeyboardAftertouch)
{
    auto address_pattern = "/keyboard_event/sampler";

    _module_under_test->add_method(address_pattern,
                                   "sif",
                                 sushi::osc::OscMethodType::SEND_KEYBOARD_MODULATION_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "aftertouch" << 11 << 0.11f << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(11, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.11f, std::stof(args["value"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/keyboard_event/drums") << "aftertouch" << 12 << 0.52f << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSendProgramChange)
{
    auto address_pattern = "/program/sampler";

    _module_under_test->add_method(address_pattern,
                                   "i", sushi::osc::OscMethodType::SEND_PROGRAM_CHANGE_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << 1 << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.program_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(1, std::stoi(args["program id"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/program/drums") << 10 << oscpack::EndMessage;
    oscpack::ReceivedPacket received_packet2(p2.Data(), p2.Size());
    oscpack::ReceivedMessage message2(received_packet2);
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSetBypassState)
{
    auto address_pattern = "/bypass/sampler";

    _module_under_test->add_method(address_pattern,
                                   "i", sushi::osc::OscMethodType::SEND_BYPASS_STATE_EVENT,
                                   &connection);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << 1 << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.audio_graph_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(1, std::stoi(args["bypass enabled"]));

    // Test with a path not registered
    _mock_controller.clear_recent_call();
    oscpack::OutboundPacketStream p2(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p2 << oscpack::BeginMessage("/bypass/drums") << 1 << oscpack::EndMessage;
    oscpack::ReceivedMessage message2(oscpack::ReceivedPacket(p2.Data(), p2.Size()));
    _module_under_test->ProcessMessage(message2, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_FALSE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestSetTempo)
{
    auto address_pattern = "/engine/set_tempo";

    _module_under_test->add_method(address_pattern,
                                   "f", sushi::osc::OscMethodType::SET_TEMPO,
                                   &_mock_controller);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << 136.0f << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.transport_controller_mockup()->get_args_from_last_call();

    ASSERT_TRUE(_mock_controller.was_recently_called());

    EXPECT_EQ(136.0f, std::stoi(args["tempo"]));
}

TEST_F(TestOscpackOscMessenger, TestSetTimeSignature)
{
    auto address_pattern = "/engine/set_time_signature";

    _module_under_test->add_method(address_pattern,
                                   "ii", sushi::osc::OscMethodType::SET_TIME_SIGNATURE,
                                   &_mock_controller);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << 7 << 8 << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.transport_controller_mockup()->get_args_from_last_call();

    ASSERT_TRUE(_mock_controller.was_recently_called());

    EXPECT_EQ(7, std::stoi(args["numerator"]));
    EXPECT_EQ(8, std::stoi(args["denominator"]));
}

TEST_F(TestOscpackOscMessenger, TestSetPlayingMode)
{
    auto address_pattern = "/engine/set_playing_mode";

    _module_under_test->add_method(address_pattern,
                                   "s", sushi::osc::OscMethodType::SET_PLAYING_MODE,
                                   &_mock_controller);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "playing" << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.transport_controller_mockup()->get_args_from_last_call();

    ASSERT_TRUE(_mock_controller.was_recently_called());

    EXPECT_EQ("PLAYING", args["playing mode"]);
}

TEST_F(TestOscpackOscMessenger, TestSetSyncMode)
{
    auto address_pattern = "/engine/set_sync_mode";

    _module_under_test->add_method(address_pattern,
                                   "s", sushi::osc::OscMethodType::SET_TEMPO_SYNC_MODE,
                                   &_mock_controller);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "midi" << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.transport_controller_mockup()->get_args_from_last_call();

    ASSERT_TRUE(_mock_controller.was_recently_called());

    EXPECT_EQ("MIDI", args["sync mode"]);
}

TEST_F(TestOscpackOscMessenger, TestSetTimingStatisticsEnabled)
{
    auto address_pattern = "/engine/set_timing_statistics_enabled";

    _module_under_test->add_method(address_pattern,
                                   "i",
                                 sushi::osc::OscMethodType::SET_TIMING_STATISTICS_ENABLED,
                                   &_mock_controller);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << 1 << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    auto args = _mock_controller.timing_controller_mockup()->get_args_from_last_call();

    ASSERT_TRUE(_mock_controller.was_recently_called());

    EXPECT_EQ("1", args["enabled"]);
}

TEST_F(TestOscpackOscMessenger, TestResetAllTimings)
{
    auto address_pattern = "/engine/reset_timing_statistics";

    _module_under_test->add_method(address_pattern,
                                   "s",
                                 sushi::osc::OscMethodType::RESET_TIMING_STATISTICS,
                                   &_mock_controller);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "all" << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));
    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_TRUE(_mock_controller.was_recently_called());
}

TEST_F(TestOscpackOscMessenger, TestResetProcessorTimings)
{
    auto address_pattern = "/engine/reset_timing_statistics";

    _module_under_test->add_method(address_pattern,
                                   "ss",
                                 sushi::osc::OscMethodType::RESET_TIMING_STATISTICS,
                                   &_mock_controller);

    oscpack::OutboundPacketStream p(_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << "processor" << "sampler" << oscpack::EndMessage;
    oscpack::ReceivedMessage message(oscpack::ReceivedPacket(p.Data(), p.Size()));

    _module_under_test->ProcessMessage(message, reinterpret_cast<const IpEndpointName&>(_endpoint));

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.timing_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor_id"]));
}

TEST_F(TestOscpackOscMessenger, TestSendFloat)
{
    auto address_pattern = "/an/osc/message";

    EXPECT_CALL(*(_module_under_test->_transmit_socket.get()), Send(_, _)).Times(1);

    _module_under_test->send(address_pattern, 0.5f);
}

TEST_F(TestOscpackOscMessenger, TestSendInt)
{
    auto address_pattern = "/an/osc/message";

    EXPECT_CALL(*(_module_under_test->_transmit_socket.get()), Send(_, _)).Times(1);

    _module_under_test->send(address_pattern, 5);
}
