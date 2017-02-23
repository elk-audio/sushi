/**
 * @brief Utility functions for decoding raw midi data.
 *        To decode raw midi messages, first call decode_message_type
 *        which returns the type and checks that the length is correct
 *        for the type. Then you can safely call the corresponding
 *        decode function for that type of message.
 */

#ifndef SUSHI_MIDI_DECODER_H
#define SUSHI_MIDI_DECODER_H

namespace sushi {
namespace midi {

/**
 * @brief Enum to represent a midi message type.
 */
enum class MessageType
{
    NOTE_OFF,
    NOTE_ON,
    POLY_KEY_PRESSURE,
    CONTROL_CHANGE,
    PROGRAM_CHANGE,
    CHANNEL_PRESSURE,
    PITCH_BEND,
    CHANNEL_MODE_MSG,
    SYSTEM_EXCLUSIVE,
    TIME_CODE,
    SONG_POSITION,
    SONG_SELECT,
    TUNE_REQUEST,
    END_OF_EXCLUSIVE,
    UNSUPPORTED,
    UNKNOWN
};

struct NoteOffMessage
{
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
};

struct NoteOnMessage
{
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
};

struct PolyKeyPressureMessage
{
    uint8_t channel;
    uint8_t note;
    uint8_t pressure;
};

struct ControlChangeMessage
{
    uint8_t channel;
    uint8_t controller;
    uint8_t value;
};

struct ProgramChangeMessage
{
    uint8_t channel;
    uint8_t program;
};

struct ChannelPressureMessage
{
    uint8_t channel;
    uint8_t pressure;
};

struct PitchBendMessage
{
    uint8_t channel;
    uint16_t value;
};



/**
 * @brief Decode the type of midi message from a byte string
 * @param data Midi data.
 * @param size Length of midi data.
 * @return The decoded type of data.
 */
MessageType decode_message_type(const uint8_t* data, size_t size);

/**
 * @brief Decode a midi note off message.
 * @param data Message data.
 * @return Decoded message.
 */
NoteOffMessage decode_note_off(const uint8_t* data);

/**
 * @brief Decode a midi note on message.
 * @param data Message data.
 * @return Decoded message.
 */
NoteOnMessage decode_note_on(const uint8_t* data);

/**
 * @brief Decode a midi control change message.
 * @param data Message data.
 * @return Decoded message.
 */
ControlChangeMessage decode_control_change(const uint8_t* data);

/**
 * @brief Decode a midi polyphonic key pressure (aftertouch) message.
 * @param data Message data.
 * @return Decoded message.
 */
PolyKeyPressureMessage decode_poly_key_pressure(const uint8_t* data);

/**
 * @brief Decode a midi program change message.
 * @param data Message data.
 * @return Decoded message.
 */
ProgramChangeMessage decode_program_change(const uint8_t* data);

/**
 * @brief Decode a midi channel pressure (non-polyphonic aftertouch) message.
 * @param data Message data.
 * @return Decoded message.
 */
ChannelPressureMessage decode_channel_pressure(const uint8_t* data);

/**
 * @brief Decode a midi pitch bend message.
 * @param data Message data.
 * @return Decoded message.
 */
PitchBendMessage decode_pitch_bend(const uint8_t* data);

} // end namespace midi
} // end namespace sushi

#endif //SUSHI_MIDI_DECODER_H
