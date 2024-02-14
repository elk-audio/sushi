#include "gtest/gtest.h"

#include <elk-warning-suppressor/warning_suppressor.hpp>

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
ELK_POP_WARNING

#include "library/midi_decoder.cpp"

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::midi;

const MidiDataByte TEST_NOTE_OFF_MSG  = {0x81, 60, 45, 0};
const MidiDataByte TEST_NOTE_ON_MSG   = {0x92, 62, 55, 0};
const MidiDataByte TEST_POLY_PRES_MSG = {0xA3, 70, 65, 0};
const MidiDataByte TEST_CTRL_CH_MSG   = {0xB4, 67, 75, 0};
const MidiDataByte TEST_PROG_CH_MSG   = {0xC5, 18, 0, 0};
const MidiDataByte TEST_CHAN_PRES_MSG = {0xD6, 16, 0, 0};
const MidiDataByte TEST_PITCH_B_MSG   = {0xE7, 8, 1, 0};
const MidiDataByte TEST_TIME_CODE_MSG = {0xF1, 0x35, 0, 0};
const MidiDataByte TEST_SONG_POS_MSG  = {0xF2, 0x05, 0x02, 0};
const MidiDataByte TEST_SONG_SEL_MSG  = {0xF3, 35, 0, 0};
const MidiDataByte TEST_CLOCK_MSG     = {0xF8, 0, 0, 0};
const MidiDataByte TEST_START_MSG     = {0xFA, 0, 0, 0};
const MidiDataByte TEST_CONTINUE_MSG  = {0xFB, 0, 0, 0};
const MidiDataByte TEST_STOP_MSG      = {0xFC, 0, 0, 0};
const MidiDataByte TEST_ACTIVE_SNS_MSG= {0xFE, 0, 0, 0};
const MidiDataByte TEST_RESET_MSG     = {0xFF, 0, 0, 0};
const MidiDataByte TEST_UNKNOWN_MSG   = {0, 0, 0, 0};

TEST (MidiDecoderTest, TestDecodeMessageType) {
    EXPECT_EQ(MessageType::NOTE_OFF, decode_message_type(TEST_NOTE_OFF_MSG));
    EXPECT_EQ(MessageType::NOTE_ON, decode_message_type(TEST_NOTE_ON_MSG));
    EXPECT_EQ(MessageType::POLY_KEY_PRESSURE, decode_message_type(TEST_POLY_PRES_MSG));
    EXPECT_EQ(MessageType::CONTROL_CHANGE, decode_message_type(TEST_CTRL_CH_MSG));
    EXPECT_EQ(MessageType::PROGRAM_CHANGE, decode_message_type(TEST_PROG_CH_MSG));
    EXPECT_EQ(MessageType::CHANNEL_PRESSURE, decode_message_type(TEST_CHAN_PRES_MSG));
    EXPECT_EQ(MessageType::PITCH_BEND, decode_message_type(TEST_PITCH_B_MSG));
    EXPECT_EQ(MessageType::TIME_CODE, decode_message_type(TEST_TIME_CODE_MSG));
    EXPECT_EQ(MessageType::SONG_POSITION, decode_message_type(TEST_SONG_POS_MSG));
    EXPECT_EQ(MessageType::SONG_SELECT, decode_message_type(TEST_SONG_SEL_MSG));

    // Realtime messages that only consist of 1 byte:
    EXPECT_EQ(MessageType::TIMING_CLOCK, decode_message_type(TEST_CLOCK_MSG));
    EXPECT_EQ(MessageType::START, decode_message_type(TEST_START_MSG));
    EXPECT_EQ(MessageType::CONTINUE, decode_message_type(TEST_CONTINUE_MSG));
    EXPECT_EQ(MessageType::STOP, decode_message_type(TEST_STOP_MSG));
    EXPECT_EQ(MessageType::ACTIVE_SENSING, decode_message_type(TEST_ACTIVE_SNS_MSG));
    EXPECT_EQ(MessageType::RESET, decode_message_type(TEST_RESET_MSG));

    // If message is incomplete it should not be recognized.
    EXPECT_EQ(MessageType::UNKNOWN, decode_message_type(TEST_UNKNOWN_MSG));
}

TEST (MidiDecoderTest, TestDecodeChannel)
{
    EXPECT_EQ(5, decode_channel({0x35, 0, 0, 0}));
}

TEST (MidiDecoderTest, TestDecodeNoteOff)
{
    NoteOffMessage msg = decode_note_off(TEST_NOTE_OFF_MSG);
    EXPECT_EQ(1, msg.channel);
    EXPECT_EQ(60, msg.note);
    EXPECT_EQ(45, msg.velocity);
}

TEST (MidiDecoderTest, TestDecodeNoteOn)
{
    NoteOnMessage msg = decode_note_on(TEST_NOTE_ON_MSG);
    EXPECT_EQ(2, msg.channel);
    EXPECT_EQ(62, msg.note);
    EXPECT_EQ(55, msg.velocity);
}

TEST (MidiDecoderTest, TestDecodePolyKeyPressure)
{
    PolyKeyPressureMessage msg = decode_poly_key_pressure(TEST_POLY_PRES_MSG);
    EXPECT_EQ(3, msg.channel);
    EXPECT_EQ(70, msg.note);
    EXPECT_EQ(65, msg.pressure);
}

TEST (ControlChangeMessage, TestDecodeControlChange)
{
    ControlChangeMessage msg = decode_control_change(TEST_CTRL_CH_MSG);
    EXPECT_EQ(4, msg.channel);
    EXPECT_EQ(67, msg.controller);
    EXPECT_EQ(75, msg.value);
}

TEST (ProgramChangeMessage, TestDecodeProgramChange)
{
    ProgramChangeMessage msg = decode_program_change(TEST_PROG_CH_MSG);
    EXPECT_EQ(5, msg.channel);
    EXPECT_EQ(18, msg.program);
}

TEST (MidiDecoderTest, TestDecodeChannelPressure)
{
    ChannelPressureMessage msg = decode_channel_pressure(TEST_CHAN_PRES_MSG);
    EXPECT_EQ(6, msg.channel);
    EXPECT_EQ(16, msg.pressure);
}

TEST (MidiDecoderTest, TestDecodePitchBend)
{
    PitchBendMessage msg = decode_pitch_bend(TEST_PITCH_B_MSG);
    EXPECT_EQ(7, msg.channel);
    EXPECT_EQ(136, msg.value);
}

TEST (MidiDecoderTest, TestDecodeTimeCode)
{
    TimeCodeMessage msg = decode_time_code(TEST_TIME_CODE_MSG);
    EXPECT_EQ(3, msg.message_type);
    EXPECT_EQ(5, msg.value);
}

TEST (MidiDecoderTest, TestDecodeSongPosition)
{
    SongPositionMessage msg = decode_song_position(TEST_SONG_POS_MSG);
    EXPECT_EQ(261, msg.beats);
}

TEST (MidiDecoderTest, TestDecodeSongSelect)
{
    SongSelectMessage msg = decode_song_select(TEST_SONG_SEL_MSG);
    EXPECT_EQ(35, msg.index);
}
