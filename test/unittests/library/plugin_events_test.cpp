#include "gtest/gtest.h"
#define private public

#include "library/plugin_events.h"

using namespace sushi;


TEST (TestPluginEvents, TestFactoryFunction)
{
    auto event = Event::make_note_on_event(123, 1, 46, 0.5);
    EXPECT_EQ(EventType::NOTE_ON, event.type());
    auto note_on_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(123), note_on_event->processor_id());
    EXPECT_EQ(1, note_on_event->sample_offset());
    EXPECT_EQ(46, note_on_event->note());
    EXPECT_FLOAT_EQ(0.5, note_on_event->velocity());

    event = Event::make_note_off_event(122, 2, 47, 0.5);
    EXPECT_EQ(EventType::NOTE_OFF, event.type());
    auto note_off_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(122), note_off_event->processor_id());
    EXPECT_EQ(2, note_off_event->sample_offset());
    EXPECT_EQ(47, note_off_event->note());
    EXPECT_FLOAT_EQ(0.5, note_off_event->velocity());

    event = Event::make_note_aftertouch_event(124, 3, 48, 0.5);
    EXPECT_EQ(EventType::NOTE_AFTERTOUCH, event.type());
    auto note_at_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(124), note_at_event->processor_id());
    EXPECT_EQ(3, note_at_event->sample_offset());
    EXPECT_EQ(48, note_at_event->note());
    EXPECT_FLOAT_EQ(0.5, note_at_event->velocity());

    event = Event::make_parameter_change_event(125, 4, 64, 0.5);
    EXPECT_EQ(EventType::FLOAT_PARAMETER_CHANGE, event.type());
    auto pc_event = event.parameter_change_event();
    EXPECT_EQ(ObjectId(125), pc_event->processor_id());
    EXPECT_EQ(4, pc_event->sample_offset());
    EXPECT_EQ(ObjectId(64), pc_event->param_id());
    EXPECT_FLOAT_EQ(0.5, pc_event->value());

    event = Event::make_wrapped_midi_event(126, 5, 6, 7, 8);
    EXPECT_EQ(EventType::WRAPPED_MIDI_EVENT, event.type());
    auto wm_event = event.wrapper_midi_event();
    EXPECT_EQ(ObjectId(126), wm_event->processor_id());
    EXPECT_EQ(5, wm_event->sample_offset());
    EXPECT_EQ(6, wm_event->midi_byte_0());
    EXPECT_EQ(7, wm_event->midi_byte_1());
    EXPECT_EQ(8, wm_event->midi_byte_2());

    std::string str("Hej");
    event = Event::make_string_parameter_change_event(127, 6, 65, &str);
    EXPECT_EQ(EventType::STRING_PARAMETER_CHANGE, event.type());
    auto spc_event = event.string_parameter_change_event();
    EXPECT_EQ(ObjectId(127), spc_event->processor_id());
    EXPECT_EQ(6, spc_event->sample_offset());
    EXPECT_EQ(ObjectId(65), spc_event->param_id());
    EXPECT_EQ("Hej", *spc_event->value());

    event = Event::make_data_parameter_change_event(128, 7, 66, static_cast<void*>(&str));
    EXPECT_EQ(EventType::DATA_PARAMETER_CHANGE, event.type());
    auto dpc_event = event.data_parameter_change_event();
    EXPECT_EQ(ObjectId(128), dpc_event->processor_id());
    EXPECT_EQ(7, dpc_event->sample_offset());
    EXPECT_EQ(ObjectId(66), dpc_event->param_id());
    EXPECT_EQ(static_cast<void*>(&str), dpc_event->value());
}


