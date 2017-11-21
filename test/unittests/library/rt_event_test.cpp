#include "gtest/gtest.h"
#define private public

#include "library/rt_event.h"

using namespace sushi;

TEST (TestPluginEvents, TestFactoryFunction)
{
    auto event = RtEvent::make_note_on_event(123, 1, 46, 0.5);
    EXPECT_EQ(RtEventType::NOTE_ON, event.type());
    auto note_on_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(123), note_on_event->processor_id());
    EXPECT_EQ(1, note_on_event->sample_offset());
    EXPECT_EQ(46, note_on_event->note());
    EXPECT_FLOAT_EQ(0.5, note_on_event->velocity());

    event = RtEvent::make_note_off_event(122, 2, 47, 0.5);
    EXPECT_EQ(RtEventType::NOTE_OFF, event.type());
    auto note_off_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(122), note_off_event->processor_id());
    EXPECT_EQ(2, note_off_event->sample_offset());
    EXPECT_EQ(47, note_off_event->note());
    EXPECT_FLOAT_EQ(0.5, note_off_event->velocity());

    event = RtEvent::make_note_aftertouch_event(124, 3, 48, 0.5);
    EXPECT_EQ(RtEventType::NOTE_AFTERTOUCH, event.type());
    auto note_at_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(124), note_at_event->processor_id());
    EXPECT_EQ(3, note_at_event->sample_offset());
    EXPECT_EQ(48, note_at_event->note());
    EXPECT_FLOAT_EQ(0.5, note_at_event->velocity());

    event = RtEvent::make_parameter_change_event(125, 4, 64, 0.5);
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
    auto pc_event = event.parameter_change_event();
    EXPECT_EQ(ObjectId(125), pc_event->processor_id());
    EXPECT_EQ(4, pc_event->sample_offset());
    EXPECT_EQ(ObjectId(64), pc_event->param_id());
    EXPECT_FLOAT_EQ(0.5, pc_event->value());

    event = RtEvent::make_wrapped_midi_event(126, 5, {6u, 7u, 8u, 0u});
    EXPECT_EQ(RtEventType::WRAPPED_MIDI_EVENT, event.type());
    auto wm_event = event.wrapper_midi_event();
    EXPECT_EQ(ObjectId(126), wm_event->processor_id());
    EXPECT_EQ(5, wm_event->sample_offset());
    EXPECT_EQ(6, wm_event->midi_data()[0]);
    EXPECT_EQ(7, wm_event->midi_data()[1]);
    EXPECT_EQ(8, wm_event->midi_data()[2]);

    std::string str("Hej");
    event = RtEvent::make_string_parameter_change_event(127, 6, 65, &str);
    EXPECT_EQ(RtEventType::STRING_PARAMETER_CHANGE, event.type());
    auto spc_event = event.string_parameter_change_event();
    EXPECT_EQ(ObjectId(127), spc_event->processor_id());
    EXPECT_EQ(6, spc_event->sample_offset());
    EXPECT_EQ(ObjectId(65), spc_event->param_id());
    EXPECT_EQ("Hej", *spc_event->value());

    uint8_t TEST_DATA[3] = {1,2,3};
    BlobData data{sizeof(TEST_DATA), TEST_DATA};
    event = RtEvent::make_data_parameter_change_event(128, 7, 66, data);
    EXPECT_EQ(RtEventType::DATA_PARAMETER_CHANGE, event.type());
    auto dpc_event = event.data_parameter_change_event();
    EXPECT_EQ(ObjectId(128), dpc_event->processor_id());
    EXPECT_EQ(7, dpc_event->sample_offset());
    EXPECT_EQ(ObjectId(66), dpc_event->param_id());
    EXPECT_EQ(3, dpc_event->value().data[2]);

    event = RtEvent::make_bypass_processor_event(130, true);
    EXPECT_EQ(RtEventType::SET_BYPASS, event.type());
    EXPECT_EQ(130u, event.processor_id());
    EXPECT_TRUE(event.processor_command_event()->value());

    event = RtEvent::make_stop_engine_event();
    EXPECT_EQ(RtEventType::STOP_ENGINE, event.type());

    event = RtEvent::make_insert_processor_event(nullptr);
    EXPECT_EQ(RtEventType::INSERT_PROCESSOR, event.type());
    EXPECT_EQ(nullptr, event.processor_operation_event()->instance());

    event = RtEvent::make_remove_processor_event(123u);
    EXPECT_EQ(RtEventType::REMOVE_PROCESSOR, event.type());
    EXPECT_EQ(123u, event.processor_reorder_event()->processor());
    EXPECT_EQ(0u, event.processor_reorder_event()->track());

    event = RtEvent::make_add_processor_to_track_event(ObjectId(123), ObjectId(456));
    EXPECT_EQ(RtEventType::ADD_PROCESSOR_TO_TRACK, event.type());
    EXPECT_EQ(123u, event.processor_reorder_event()->processor());
    EXPECT_EQ(456u, event.processor_reorder_event()->track());

    event = RtEvent::make_remove_processor_from_track_event(ObjectId(123), ObjectId(456));
    EXPECT_EQ(RtEventType::REMOVE_PROCESSOR_FROM_TRACK, event.type());
    EXPECT_EQ(123u, event.processor_reorder_event()->processor());
    EXPECT_EQ(456u, event.processor_reorder_event()->track());
}

TEST(TestPluginEvents, TestReturnableEvents)
{
    auto event = RtEvent::make_stop_engine_event();
    auto event2 = RtEvent::make_stop_engine_event();
    auto typed_event = event.returnable_event();
    /* Assert that 2 events don't share the same id */
    EXPECT_NE(event2.returnable_event()->event_id(), typed_event->event_id());
    /* Verify handling logic */
    EXPECT_EQ(ReturnableRtEvent::EventStatus::UNHANDLED, typed_event->status());
    typed_event->set_handled(true);
    EXPECT_EQ(ReturnableRtEvent::EventStatus::HANDLED_OK, typed_event->status());
    typed_event->set_handled(false);
    EXPECT_EQ(ReturnableRtEvent::EventStatus::HANDLED_ERROR, typed_event->status());
}


