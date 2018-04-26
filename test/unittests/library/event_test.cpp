#include "gtest/gtest.h"
#define private public

#include "library/event.cpp"

using namespace sushi;

static int dummy_processor_callback(void* /*arg*/, EventId /*id*/)
{
    return 0;
}

TEST(EventTest, TestToRtEvent)
{
    auto note_on_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, 1, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_on_event.is_keyboard_event());
    EXPECT_TRUE(note_on_event.maps_to_rt_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, note_on_event.time());
    RtEvent rt_event = note_on_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_ON, rt_event.type());
    EXPECT_EQ(5, rt_event.sample_offset());
    EXPECT_EQ(1u, rt_event.keyboard_event()->processor_id());
    EXPECT_EQ(48, rt_event.keyboard_event()->note());
    EXPECT_FLOAT_EQ(1.0f, rt_event.keyboard_event()->velocity());

    auto note_off_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, 1, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_off_event.is_keyboard_event());
    EXPECT_TRUE(note_off_event.maps_to_rt_event());
    rt_event = note_off_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_OFF, rt_event.type());

    auto note_at_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, 1, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_at_event.is_keyboard_event());
    EXPECT_TRUE(note_at_event.maps_to_rt_event());
    rt_event = note_at_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_AFTERTOUCH, rt_event.type());;

    auto pitchbend_event = KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, 2, 0.5f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(pitchbend_event.is_keyboard_event());
    rt_event = pitchbend_event.to_rt_event(6);
    EXPECT_EQ(RtEventType::PITCH_BEND, rt_event.type());
    EXPECT_EQ(6, rt_event.sample_offset());
    EXPECT_EQ(2u, rt_event.keyboard_common_event()->processor_id());
    EXPECT_FLOAT_EQ(0.5f, rt_event.keyboard_common_event()->value());

    auto modulation_event = KeyboardEvent(KeyboardEvent::Subtype::MODULATION, 3, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(modulation_event.is_keyboard_event());
    EXPECT_TRUE(modulation_event.maps_to_rt_event());
    rt_event = modulation_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::MODULATION, rt_event.type());

    auto aftertouch_event = KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, 4, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(aftertouch_event.is_keyboard_event());
    EXPECT_TRUE(aftertouch_event.maps_to_rt_event());
    rt_event = aftertouch_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::AFTERTOUCH, rt_event.type());

    auto midi_event = KeyboardEvent(KeyboardEvent::Subtype::WRAPPED_MIDI, 5, {1,2,3,4}, IMMEDIATE_PROCESS);
    EXPECT_TRUE(midi_event.is_keyboard_event());
    rt_event = midi_event.to_rt_event(7);
    EXPECT_EQ(RtEventType::WRAPPED_MIDI_EVENT, rt_event.type());
    EXPECT_EQ(7, rt_event.sample_offset());
    EXPECT_EQ(5u, rt_event.wrapped_midi_event()->processor_id());
    EXPECT_EQ(MidiDataByte({1,2,3,4}), rt_event.wrapped_midi_event()->midi_data());

    auto param_ch_event = ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE, 6, 50, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(param_ch_event.is_parameter_change_event());
    EXPECT_TRUE(param_ch_event.maps_to_rt_event());
    rt_event = param_ch_event.to_rt_event(8);
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, rt_event.type());
    EXPECT_EQ(8, rt_event.sample_offset());
    EXPECT_EQ(6u, rt_event.parameter_change_event()->processor_id());
    EXPECT_EQ(50u, rt_event.parameter_change_event()->param_id());
    EXPECT_FLOAT_EQ(1.0f, rt_event.parameter_change_event()->value());

    auto string_pro_ch_event = StringPropertyChangeEvent(7, 51, "Hello", IMMEDIATE_PROCESS);
    EXPECT_TRUE(string_pro_ch_event.is_parameter_change_event());
    EXPECT_TRUE(string_pro_ch_event.maps_to_rt_event());
    rt_event = string_pro_ch_event.to_rt_event(10);
    EXPECT_EQ(RtEventType::STRING_PROPERTY_CHANGE, rt_event.type());
    EXPECT_EQ(10, rt_event.sample_offset());
    EXPECT_EQ(7u, rt_event.string_parameter_change_event()->processor_id());
    EXPECT_EQ(51u, rt_event.string_parameter_change_event()->param_id());
    EXPECT_STREQ("Hello", rt_event.string_parameter_change_event()->value()->c_str());

    BlobData testdata = {0, nullptr};
    auto data_pro_ch_event = DataPropertyChangeEvent(8, 52, testdata, IMMEDIATE_PROCESS);
    EXPECT_TRUE(data_pro_ch_event.is_parameter_change_event());
    EXPECT_TRUE(data_pro_ch_event.maps_to_rt_event());
    rt_event = data_pro_ch_event.to_rt_event(10);
    EXPECT_EQ(RtEventType::DATA_PROPERTY_CHANGE, rt_event.type());
    EXPECT_EQ(10, rt_event.sample_offset());
    EXPECT_EQ(8u, rt_event.data_parameter_change_event()->processor_id());
    EXPECT_EQ(52u, rt_event.data_parameter_change_event()->param_id());
    EXPECT_EQ(0, rt_event.data_parameter_change_event()->value().size);
    EXPECT_EQ(nullptr, rt_event.data_parameter_change_event()->value().data);

    auto async_comp_not = AsynchronousProcessorWorkCompletionEvent(123, 9, 53, IMMEDIATE_PROCESS);
    rt_event = async_comp_not.to_rt_event(11);
    EXPECT_EQ(RtEventType::ASYNC_WORK_NOTIFICATION, rt_event.type());
    EXPECT_EQ(123, rt_event.async_work_completion_event()->return_status());
    EXPECT_EQ(9u, rt_event.async_work_completion_event()->processor_id());
    EXPECT_EQ(53u, rt_event.async_work_completion_event()->sending_event_id());

    auto tempo_event = SetEngineTempoEvent(135, IMMEDIATE_PROCESS);
    EXPECT_TRUE(tempo_event.maps_to_rt_event());
    rt_event = tempo_event.to_rt_event(12);
    EXPECT_EQ(RtEventType::TEMPO, rt_event.type());
    EXPECT_EQ(12, rt_event.sample_offset());
    EXPECT_FLOAT_EQ(135.0f, rt_event.tempo_event()->tempo());

    auto time_sig_event = SetEngineTimeSignatureEvent({3, 4}, IMMEDIATE_PROCESS);
    EXPECT_TRUE(time_sig_event.maps_to_rt_event());
    rt_event = time_sig_event.to_rt_event(13);
    EXPECT_EQ(RtEventType::TIME_SIGNATURE, rt_event.type());
    EXPECT_EQ(13, rt_event.sample_offset());
    EXPECT_EQ(3, rt_event.time_signature_event()->time_signature().numerator);
    EXPECT_EQ(4, rt_event.time_signature_event()->time_signature().denominator);

    auto trans_state_event = SetEnginePlayingModeStateEvent(PlayingMode::RECORDING, IMMEDIATE_PROCESS);
    EXPECT_TRUE(trans_state_event.maps_to_rt_event());
    rt_event = trans_state_event.to_rt_event(14);
    EXPECT_EQ(RtEventType::PLAYING_MODE, rt_event.type());
    EXPECT_EQ(14, rt_event.sample_offset());
    EXPECT_EQ(PlayingMode::RECORDING, rt_event.playing_mode_event()->mode());

    auto sync_mode_event = SetEngineSyncModeEvent(SyncMode::ABLETON_LINK, IMMEDIATE_PROCESS);
    EXPECT_TRUE(sync_mode_event.maps_to_rt_event());
    rt_event = sync_mode_event.to_rt_event(15);
    EXPECT_EQ(RtEventType::SYNC_MODE, rt_event.type());
    EXPECT_EQ(15, rt_event.sample_offset());
    EXPECT_EQ(SyncMode::ABLETON_LINK, rt_event.sync_mode_event()->mode());
}

TEST(EventTest, TestFromRtEvent)
{
    auto note_on_event = RtEvent::make_note_on_event(2, 0, 48, 1.0f);
    Event* event = Event::from_rt_event(note_on_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_ON, kb_event->subtype());
    EXPECT_EQ(48, kb_event->note());
    EXPECT_EQ(2u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto note_off_event = RtEvent::make_note_off_event(3, 0, 49, 1.0f);
    event = Event::from_rt_event(note_off_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_OFF, kb_event->subtype());
    EXPECT_EQ(49, kb_event->note());
    EXPECT_EQ(3u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto note_at_event = RtEvent::make_note_aftertouch_event(4, 0, 50, 1.0f);
    event = Event::from_rt_event(note_at_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, kb_event->subtype());
    EXPECT_EQ(50, kb_event->note());
    EXPECT_EQ(4u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto mod_event = RtEvent::make_kb_modulation_event(5, 0, 0.5f);
    event = Event::from_rt_event(mod_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::MODULATION, kb_event->subtype());
    EXPECT_EQ(5u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(0.5f, kb_event->value());
    delete event;

    auto pb_event = RtEvent::make_pitch_bend_event(6, 0, 0.6f);
    event = Event::from_rt_event(pb_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::PITCH_BEND, kb_event->subtype());
    EXPECT_EQ(6u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(0.6f, kb_event->value());
    delete event;

    auto at_event = RtEvent::make_aftertouch_event(7, 0, 0.7f);
    event = Event::from_rt_event(at_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::AFTERTOUCH, kb_event->subtype());
    EXPECT_EQ(7u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(0.7f, kb_event->value());
    delete event;

    auto midi_event = RtEvent::make_wrapped_midi_event(8, 0, {1,2,3,4});
    event = Event::from_rt_event(midi_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::WRAPPED_MIDI, kb_event->subtype());
    EXPECT_EQ(8u, kb_event->processor_id());
    EXPECT_EQ(MidiDataByte({1,2,3,4}), kb_event->midi_data());
    delete event;

    auto param_ch_event = RtEvent::make_parameter_change_event(9, 0, 50, 0.1f);
    event = Event::from_rt_event(param_ch_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_parameter_change_notification());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto pc_event = static_cast<ParameterChangeNotificationEvent*>(event);
    EXPECT_EQ(ParameterChangeNotificationEvent::Subtype::FLOAT_PARAMETER_CHANGE_NOT, pc_event->subtype());
    EXPECT_EQ(9u, pc_event->processor_id());
    EXPECT_EQ(50u, pc_event->parameter_id());
    EXPECT_FLOAT_EQ(0.1f, pc_event->float_value());
    delete event;

    auto async_work_event = RtEvent::make_async_work_event(dummy_processor_callback, 10, nullptr);
    event = Event::from_rt_event(async_work_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_async_work_event());
    EXPECT_TRUE(event->process_asynchronously());
    delete event;

    BlobData testdata = {0, nullptr};
    auto async_blod_del_event = RtEvent::make_delete_blob_event(testdata);
    event = Event::from_rt_event(async_blod_del_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_async_work_event());
    EXPECT_TRUE(event->process_asynchronously());
    delete event;
}