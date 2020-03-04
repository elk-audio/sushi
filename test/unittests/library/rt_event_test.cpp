#include "gtest/gtest.h"
#define private public

#include "library/rt_event.h"

using namespace sushi;

TEST (TestRealtimeEvents, TestFactoryFunction)
{
    auto event = RtEvent::make_note_on_event(123, 1, 0, 46, 0.5);
    EXPECT_EQ(RtEventType::NOTE_ON, event.type());
    auto note_on_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(123), note_on_event->processor_id());
    EXPECT_EQ(1, note_on_event->sample_offset());
    EXPECT_EQ(46, note_on_event->note());
    EXPECT_FLOAT_EQ(0.5, note_on_event->velocity());

    event = RtEvent::make_note_off_event(122, 2, 0, 47, 0.5);
    EXPECT_EQ(RtEventType::NOTE_OFF, event.type());
    auto note_off_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(122), note_off_event->processor_id());
    EXPECT_EQ(2, note_off_event->sample_offset());
    EXPECT_EQ(47, note_off_event->note());
    EXPECT_FLOAT_EQ(0.5, note_off_event->velocity());

    event = RtEvent::make_note_aftertouch_event(124, 3, 0, 48, 0.5);
    EXPECT_EQ(RtEventType::NOTE_AFTERTOUCH, event.type());
    auto note_at_event = event.keyboard_event();
    EXPECT_EQ(ObjectId(124), note_at_event->processor_id());
    EXPECT_EQ(3, note_at_event->sample_offset());
    EXPECT_EQ(48, note_at_event->note());
    EXPECT_FLOAT_EQ(0.5, note_at_event->velocity());

    event = RtEvent::make_aftertouch_event(111, 3, 0, 0.6);
    EXPECT_EQ(RtEventType::AFTERTOUCH, event.type());
    auto at_event = event.keyboard_common_event();
    EXPECT_EQ(ObjectId(111), at_event->processor_id());
    EXPECT_EQ(3, at_event->sample_offset());
    EXPECT_FLOAT_EQ(0.6, at_event->value());

    event = RtEvent::make_pitch_bend_event(112, 4, 0, 0.7);
    EXPECT_EQ(RtEventType::PITCH_BEND, event.type());
    auto pb_event = event.keyboard_common_event();
    EXPECT_EQ(ObjectId(112), pb_event->processor_id());
    EXPECT_EQ(4, pb_event->sample_offset());
    EXPECT_FLOAT_EQ(0.7, pb_event->value());

    event = RtEvent::make_kb_modulation_event(113, 5, 0, 0.8);
    EXPECT_EQ(RtEventType::MODULATION, event.type());
    auto mod_event = event.keyboard_common_event();
    EXPECT_EQ(ObjectId(113), mod_event->processor_id());
    EXPECT_EQ(5, mod_event->sample_offset());
    EXPECT_FLOAT_EQ(0.8, mod_event->value());

    event = RtEvent::make_parameter_change_event(125, 4, 64, 0.5);
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
    auto pc_event = event.parameter_change_event();
    EXPECT_EQ(ObjectId(125), pc_event->processor_id());
    EXPECT_EQ(4, pc_event->sample_offset());
    EXPECT_EQ(ObjectId(64), pc_event->param_id());
    EXPECT_FLOAT_EQ(0.5, pc_event->value());

    event = RtEvent::make_wrapped_midi_event(126, 5, {6u, 7u, 8u, 0u});
    EXPECT_EQ(RtEventType::WRAPPED_MIDI_EVENT, event.type());
    auto wm_event = event.wrapped_midi_event();
    EXPECT_EQ(ObjectId(126), wm_event->processor_id());
    EXPECT_EQ(5, wm_event->sample_offset());
    EXPECT_EQ(6, wm_event->midi_data()[0]);
    EXPECT_EQ(7, wm_event->midi_data()[1]);
    EXPECT_EQ(8, wm_event->midi_data()[2]);

    event = RtEvent::make_gate_event(127, 6, 1, true);
    EXPECT_EQ(RtEventType::GATE_EVENT, event.type());
    auto gate_event = event.gate_event();
    EXPECT_EQ(ObjectId(127), gate_event->processor_id());
    EXPECT_EQ(6, gate_event->sample_offset());
    EXPECT_EQ(1, gate_event->gate_no());
    EXPECT_TRUE(gate_event->value());

    event = RtEvent::make_cv_event(128, 7, 2, 0.5);
    EXPECT_EQ(RtEventType::CV_EVENT, event.type());
    auto cv_event = event.cv_event();
    EXPECT_EQ(ObjectId(128), cv_event->processor_id());
    EXPECT_EQ(7, cv_event->sample_offset());
    EXPECT_EQ(2, cv_event->cv_id());
    EXPECT_FLOAT_EQ(0.5, cv_event->value());

    std::string str("Hej");
    event = RtEvent::make_string_parameter_change_event(129, 8, 65, &str);
    EXPECT_EQ(RtEventType::STRING_PROPERTY_CHANGE, event.type());
    auto spc_event = event.string_parameter_change_event();
    EXPECT_EQ(ObjectId(129), spc_event->processor_id());
    EXPECT_EQ(8, spc_event->sample_offset());
    EXPECT_EQ(ObjectId(65), spc_event->param_id());
    EXPECT_EQ("Hej", *spc_event->value());

    uint8_t TEST_DATA[3] = {1,2,3};
    BlobData data{sizeof(TEST_DATA), TEST_DATA};
    event = RtEvent::make_data_parameter_change_event(130, 9, 66, data);
    EXPECT_EQ(RtEventType::DATA_PROPERTY_CHANGE, event.type());
    auto dpc_event = event.data_parameter_change_event();
    EXPECT_EQ(ObjectId(130), dpc_event->processor_id());
    EXPECT_EQ(9, dpc_event->sample_offset());
    EXPECT_EQ(ObjectId(66), dpc_event->param_id());
    EXPECT_EQ(3, dpc_event->value().data[2]);

    event = RtEvent::make_bypass_processor_event(131, true);
    EXPECT_EQ(RtEventType::SET_BYPASS, event.type());
    EXPECT_EQ(131u, event.processor_id());
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

    event = RtEvent::make_tempo_event(25, 130);
    EXPECT_EQ(RtEventType::TEMPO, event.type());
    EXPECT_EQ(25, event.tempo_event()->sample_offset());
    EXPECT_EQ(130.0f, event.tempo_event()->tempo());

    event = RtEvent::make_time_signature_event(26, {7, 8});
    EXPECT_EQ(RtEventType::TIME_SIGNATURE, event.type());
    EXPECT_EQ(26, event.time_signature_event()->sample_offset());
    EXPECT_EQ(7, event.time_signature_event()->time_signature().numerator);
    EXPECT_EQ(8, event.time_signature_event()->time_signature().denominator);

    event = RtEvent::make_playing_mode_event(27, PlayingMode::PLAYING);
    EXPECT_EQ(RtEventType::PLAYING_MODE, event.type());
    EXPECT_EQ(27, event.playing_mode_event()->sample_offset());
    EXPECT_EQ(PlayingMode::PLAYING, event.playing_mode_event()->mode());

    event = RtEvent::make_sync_mode_event(28, SyncMode::MIDI);
    EXPECT_EQ(RtEventType::SYNC_MODE, event.type());
    EXPECT_EQ(28, event.sync_mode_event()->sample_offset());
    EXPECT_EQ(SyncMode::MIDI, event.sync_mode_event()->mode());
}

TEST(TestRealtimeEvents, TestReturnableEvents)
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

TEST(TestRealtimeEvents, TestIsKeyboardEvent)
{
    auto event = RtEvent::make_parameter_change_event(1, 2, 3, 1.0f);
    EXPECT_FALSE(is_keyboard_event(event));
    event = RtEvent::make_note_off_event(1, 2, 0, 3, 1.0f);
    EXPECT_TRUE(is_keyboard_event(event));
    event = RtEvent::make_wrapped_midi_event(1, 2, {0, 0 ,0, 0});
    EXPECT_TRUE(is_keyboard_event(event));
}

