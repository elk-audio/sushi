#include "gtest/gtest.h"
#define private public

#include "library/midi_decoder.cpp"

using namespace sushi;
using namespace midi;

const uint8_t TEST_NOTE_OFF_MSG[] = {0x81, 60, 45};
const uint8_t TEST_NOTE_ON_MSG[] = {0x92, 62, 55};
const uint8_t TEST_POLY_PRES_MSG[] = {0xA3, 70, 65};
const uint8_t TEST_CTRL_CH_MSG[] = {0xB4, 67, 75};
const uint8_t TEST_PROG_CH_MSG[] = {0xC5, 18};
const uint8_t TEST_CHAN_PRES_MSG[] = {0xD6, 16};
const uint8_t TEST_PITCH_B_MSG[] = {0xE7, 8, 1};

TEST (MidiDecoderTest, decode_message_type_test) {
    ASSERT_EQ(MessageType::NOTE_OFF, decode_message_type(TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG)));
    ASSERT_EQ(MessageType::NOTE_ON, decode_message_type(TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG)));
    ASSERT_EQ(MessageType::POLY_KEY_PRESSURE, decode_message_type(TEST_POLY_PRES_MSG, sizeof(TEST_POLY_PRES_MSG)));
    ASSERT_EQ(MessageType::CONTROL_CHANGE, decode_message_type(TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG)));
    ASSERT_EQ(MessageType::PROGRAM_CHANGE, decode_message_type(TEST_PROG_CH_MSG, sizeof(TEST_PROG_CH_MSG)));
    ASSERT_EQ(MessageType::CHANNEL_PRESSURE, decode_message_type(TEST_CHAN_PRES_MSG, sizeof(TEST_CHAN_PRES_MSG)));
    ASSERT_EQ(MessageType::PITCH_BEND, decode_message_type(TEST_PITCH_B_MSG, sizeof(TEST_PITCH_B_MSG)));

    // If message is incomplete it should not be recognized.
    ASSERT_EQ(MessageType::UNKNOWN, decode_message_type(TEST_NOTE_OFF_MSG, 2));
}

TEST (MidiDecoderTest, decode_channel_test)
{
    ASSERT_EQ(5, decode_channel(0x35));
}

TEST (MidiDecoderTest, decode_note_off_test)
{
    NoteOffMessage msg = decode_note_off(TEST_NOTE_OFF_MSG);
    ASSERT_EQ(1, msg.channel);
    ASSERT_EQ(60, msg.note);
    ASSERT_EQ(45, msg.velocity);
}

TEST (MidiDecoderTest, decode_note_on_test)
{
    NoteOnMessage msg = decode_note_on(TEST_NOTE_ON_MSG);
    ASSERT_EQ(2, msg.channel);
    ASSERT_EQ(62, msg.note);
    ASSERT_EQ(55, msg.velocity);
}

TEST (MidiDecoderTest, decode_poly_key_pressure_test)
{
    PolyKeyPressureMessage msg = decode_poly_key_pressure(TEST_POLY_PRES_MSG);
    ASSERT_EQ(3, msg.channel);
    ASSERT_EQ(70, msg.note);
    ASSERT_EQ(65, msg.pressure);
}

TEST (ControlChangeMessage, decode_control_change_test)
{
    ControlChangeMessage msg = decode_control_change(TEST_CTRL_CH_MSG);
    ASSERT_EQ(4, msg.channel);
    ASSERT_EQ(67, msg.controller);
    ASSERT_EQ(75, msg.value);
}

TEST (ProgramChangeMessage, decode_program_change_test)
{
    ProgramChangeMessage msg = decode_program_change(TEST_PROG_CH_MSG);
    ASSERT_EQ(5, msg.channel);
    ASSERT_EQ(18, msg.program);
}

TEST (MidiDecoderTest, decode_channel_pressure_test)
{
    ChannelPressureMessage msg = decode_channel_pressure(TEST_CHAN_PRES_MSG);
    ASSERT_EQ(6, msg.channel);
    ASSERT_EQ(16, msg.pressure);
}

TEST (MidiDecoderTest, decode_pitch_bend_test)
{
    PitchBendMessage msg = decode_pitch_bend(TEST_PITCH_B_MSG);
    ASSERT_EQ(7, msg.channel);
    ASSERT_EQ(136, msg.value);
}

