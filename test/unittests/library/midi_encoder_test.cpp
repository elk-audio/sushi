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