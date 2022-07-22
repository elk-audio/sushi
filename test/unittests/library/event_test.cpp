#include "gtest/gtest.h"

#include "library/event.cpp"

#include "engine/audio_engine.h"

#include "control_frontends/base_control_frontend.h"

using namespace sushi;
using namespace sushi::engine;
using namespace sushi::control_frontend;
using namespace sushi::midi_dispatcher;

static int dummy_processor_callback(void* /*arg*/, EventId /*id*/)
{
    return 0;
}

TEST(EventTest, TestToRtEvent)
{
    auto note_on_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, 1, 0, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_on_event.is_keyboard_event());
    EXPECT_TRUE(note_on_event.maps_to_rt_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, note_on_event.time());
    RtEvent rt_event = note_on_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_ON, rt_event.type());
    EXPECT_EQ(5, rt_event.sample_offset());
    EXPECT_EQ(1u, rt_event.keyboard_event()->processor_id());
    EXPECT_EQ(48, rt_event.keyboard_event()->note());
    EXPECT_EQ(0, rt_event.keyboard_event()->channel());
    EXPECT_FLOAT_EQ(1.0f, rt_event.keyboard_event()->velocity());

    auto note_off_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, 1, 1, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_off_event.is_keyboard_event());
    EXPECT_TRUE(note_off_event.maps_to_rt_event());
    rt_event = note_off_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_OFF, rt_event.type());

    auto note_at_event = KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, 1, 2, 48, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(note_at_event.is_keyboard_event());
    EXPECT_TRUE(note_at_event.maps_to_rt_event());
    rt_event = note_at_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::NOTE_AFTERTOUCH, rt_event.type());

    auto pitchbend_event = KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, 2, 3, 0.5f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(pitchbend_event.is_keyboard_event());
    rt_event = pitchbend_event.to_rt_event(6);
    EXPECT_EQ(RtEventType::PITCH_BEND, rt_event.type());
    EXPECT_EQ(6, rt_event.sample_offset());
    EXPECT_EQ(2u, rt_event.keyboard_common_event()->processor_id());
    EXPECT_FLOAT_EQ(0.5f, rt_event.keyboard_common_event()->value());

    auto modulation_event = KeyboardEvent(KeyboardEvent::Subtype::MODULATION, 3, 4, 1.0f, IMMEDIATE_PROCESS);
    EXPECT_TRUE(modulation_event.is_keyboard_event());
    EXPECT_TRUE(modulation_event.maps_to_rt_event());
    rt_event = modulation_event.to_rt_event(5);
    EXPECT_EQ(RtEventType::MODULATION, rt_event.type());

    auto aftertouch_event = KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, 4, 5, 1.0f, IMMEDIATE_PROCESS);
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
    EXPECT_TRUE(param_ch_event.maps_to_rt_event());
    rt_event = param_ch_event.to_rt_event(8);
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, rt_event.type());
    EXPECT_EQ(8, rt_event.sample_offset());
    EXPECT_EQ(6u, rt_event.parameter_change_event()->processor_id());
    EXPECT_EQ(50u, rt_event.parameter_change_event()->param_id());
    EXPECT_FLOAT_EQ(1.0f, rt_event.parameter_change_event()->value());

    BlobData testdata = {0, nullptr};
    auto data_pro_ch_event = DataPropertyEvent(8, 52, testdata, IMMEDIATE_PROCESS);
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

    auto bypass_event = SetProcessorBypassEvent(10, true, IMMEDIATE_PROCESS);
    EXPECT_TRUE(bypass_event.bypass_enabled());
    rt_event = bypass_event.to_rt_event(12);
    EXPECT_EQ(RtEventType::SET_BYPASS, rt_event.type());
    EXPECT_TRUE(rt_event.processor_command_event()->value());
    EXPECT_EQ(10u, rt_event.processor_command_event()->processor_id());
}

TEST(EventTest, TestFromRtEvent)
{
    auto note_on_event = RtEvent::make_note_on_event(2, 0, 1, 48, 1.0f);
    auto event = Event::from_rt_event(note_on_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_ON, kb_event->subtype());
    EXPECT_EQ(1, kb_event->channel());
    EXPECT_EQ(48, kb_event->note());
    EXPECT_EQ(2u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto note_off_event = RtEvent::make_note_off_event(3, 0, 2, 49, 1.0f);
    event = Event::from_rt_event(note_off_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_OFF, kb_event->subtype());
    EXPECT_EQ(2, kb_event->channel());
    EXPECT_EQ(49, kb_event->note());
    EXPECT_EQ(3u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto note_at_event = RtEvent::make_note_aftertouch_event(4, 0, 3, 50, 1.0f);
    event = Event::from_rt_event(note_at_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, kb_event->subtype());
    EXPECT_EQ(3, kb_event->channel());
    EXPECT_EQ(50, kb_event->note());
    EXPECT_EQ(4u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(1.0f, kb_event->value());
    delete event;

    auto mod_event = RtEvent::make_kb_modulation_event(5, 0, 4, 0.5f);
    event = Event::from_rt_event(mod_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::MODULATION, kb_event->subtype());
    EXPECT_EQ(4, kb_event->channel());
    EXPECT_EQ(5u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(0.5f, kb_event->value());
    delete event;

    auto pb_event = RtEvent::make_pitch_bend_event(6, 0, 5, 0.6f);
    event = Event::from_rt_event(pb_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::PITCH_BEND, kb_event->subtype());
    EXPECT_EQ(5, kb_event->channel());
    EXPECT_EQ(6u, kb_event->processor_id());
    EXPECT_FLOAT_EQ(0.6f, kb_event->value());
    delete event;

    auto at_event = RtEvent::make_aftertouch_event(7, 0, 6, 0.7f);
    event = Event::from_rt_event(at_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    kb_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::AFTERTOUCH, kb_event->subtype());
    EXPECT_EQ(6, kb_event->channel());
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

    auto tempo_event = RtEvent::make_tempo_event(10, 125);
    event = Event::from_rt_event(tempo_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_engine_notification());
    auto tempo_not = static_cast<TempoNotificationEvent*>(event);
    EXPECT_TRUE(tempo_not->is_tempo_notification());
    EXPECT_FLOAT_EQ(125.0f, tempo_not->tempo());
    delete event;

    auto time_sig_event = RtEvent::make_time_signature_event(11, {.numerator = 6, .denominator = 4});
    event = Event::from_rt_event(time_sig_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_engine_notification());
    auto time_sig_not = static_cast<TimeSignatureNotificationEvent*>(event);
    EXPECT_TRUE(time_sig_not->is_time_sign_notification());
    EXPECT_EQ(6, time_sig_not->time_signature().numerator);
    EXPECT_EQ(4, time_sig_not->time_signature().denominator);
    delete event;

    auto play_mode_event = RtEvent::make_playing_mode_event(12, PlayingMode::RECORDING);
    event = Event::from_rt_event(play_mode_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_engine_notification());
    auto play_mode_not = static_cast<PlayingModeNotificationEvent*>(event);
    EXPECT_TRUE(play_mode_not->is_playing_mode_notification());
    EXPECT_EQ(PlayingMode::RECORDING, play_mode_not->mode());
    delete event;

    auto sync_mode_event = RtEvent::make_sync_mode_event(13, SyncMode::MIDI);
    event = Event::from_rt_event(sync_mode_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_engine_notification());
    auto sync_mode_not = static_cast<SyncModeNotificationEvent*>(event);
    EXPECT_TRUE(sync_mode_not->is_sync_mode_notification());
    EXPECT_EQ(SyncMode::MIDI, sync_mode_not->mode());
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

    auto notify_event = RtEvent::make_processor_notify_event(30, ProcessorNotifyRtEvent::Action::PARAMETER_UPDATE);
    event = Event::from_rt_event(notify_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_engine_notification());
    EXPECT_EQ(static_cast<AudioGraphNotificationEvent*>(event)->action(), AudioGraphNotificationEvent::Action::PROCESSOR_UPDATED);
    delete event;

    auto tick_event = RtEvent::make_timing_tick_event(14, 12);
    event = Event::from_rt_event(tick_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(event != nullptr);
    EXPECT_TRUE(event->is_engine_notification());
    auto tick_not = static_cast<EngineTimingTickNotificationEvent*>(event);
    EXPECT_TRUE(tick_not->is_timing_tick_notification());
    EXPECT_EQ(12, tick_not->tick_count());
    delete event;
}
