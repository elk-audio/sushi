#include "gtest/gtest.h"

#include "library/midi_encoder.cpp"

using namespace sushi;
using namespace midi;

TEST(TestMidiEncoder, EncodeNoteOn)
{
    auto midi_msg = encode_note_on(1, 48, 1.0f);
    EXPECT_EQ(0x91, midi_msg[0]);
    EXPECT_EQ(48u, midi_msg[1]);
    EXPECT_EQ(127u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeNoteOff)
{
    auto midi_msg = encode_note_off(2, 50, 1.0f);
    EXPECT_EQ(0x82, midi_msg[0]);
    EXPECT_EQ(50u, midi_msg[1]);
    EXPECT_EQ(127u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodePolyKeyPressure)
{
    auto midi_msg = encode_poly_key_pressure(3, 52, 1.0f);
    EXPECT_EQ(0xA3, midi_msg[0]);
    EXPECT_EQ(52u, midi_msg[1]);
    EXPECT_EQ(127u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeControlChange)
{
    auto midi_msg = encode_control_change(4, 12, 1.0f);
    EXPECT_EQ(0xB4, midi_msg[0]);
    EXPECT_EQ(12u, midi_msg[1]);
    EXPECT_EQ(127u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeChannelPressure)
{
    auto midi_msg = encode_channel_pressure(5, 1.0f);
    EXPECT_EQ(0xD5, midi_msg[0]);
    EXPECT_EQ(127u, midi_msg[1]);
    EXPECT_EQ(0u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodePitchBend)
{
    auto midi_msg = encode_pitch_bend(6, 1.0f);
    int pb = midi_msg[1] + (midi_msg[2] << 7);
    EXPECT_EQ(0xE6, midi_msg[0]);
    EXPECT_EQ(MAX_PITCH_BEND, pb);

    midi_msg = encode_pitch_bend(7, 0.0f);
    pb = midi_msg[1] + (midi_msg[2] << 7) - PITCH_BEND_MIDDLE;
    EXPECT_EQ(0xE7, midi_msg[0]);
    EXPECT_EQ(0, pb);
}

TEST(TestMidiEncoder, EncodeProgramChange)
{
    auto midi_msg = encode_program_change(7, 53);
    EXPECT_EQ(0xC7, midi_msg[0]);
    EXPECT_EQ(53u, midi_msg[1]);
    EXPECT_EQ(0u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeStartMessage)
{
    auto midi_msg = encode_start_message();
    EXPECT_EQ(0xFA, midi_msg[0]);
    EXPECT_EQ(0u, midi_msg[1]);
    EXPECT_EQ(0u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeStopMessage)
{
    auto midi_msg = encode_stop_message();
    EXPECT_EQ(0xFC, midi_msg[0]);
    EXPECT_EQ(0u, midi_msg[1]);
    EXPECT_EQ(0u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeContinueMessage)
{
    auto midi_msg = encode_continue_message();
    EXPECT_EQ(0xFB, midi_msg[0]);
    EXPECT_EQ(0u, midi_msg[1]);
    EXPECT_EQ(0u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeTimingClock)
{
    auto midi_msg = encode_timing_clock();
    EXPECT_EQ(0xF8, midi_msg[0]);
    EXPECT_EQ(0u, midi_msg[1]);
    EXPECT_EQ(0u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeActiveSensing)
{
    auto midi_msg = encode_active_sensing();
    EXPECT_EQ(0xFE, midi_msg[0]);
    EXPECT_EQ(0u, midi_msg[1]);
    EXPECT_EQ(0u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}

TEST(TestMidiEncoder, EncodeResetMessage)
{
    auto midi_msg = encode_reset_message();
    EXPECT_EQ(0xFF, midi_msg[0]);
    EXPECT_EQ(0u, midi_msg[1]);
    EXPECT_EQ(0u, midi_msg[2]);
    EXPECT_EQ(0u, midi_msg[3]);
}