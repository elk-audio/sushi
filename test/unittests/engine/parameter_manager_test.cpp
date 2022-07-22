#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "gmock/gmock-actions.h"

#include "engine/parameter_manager.cpp"
#include "plugins/gain_plugin.h"

#include "test_utils/mock_event_dispatcher.h"
#include "test_utils/mock_processor_container.h"
#include "test_utils/host_control_mockup.h"

using namespace sushi;

using ::testing::Return;
using ::testing::AllOf;
using ::testing::Field;
using ::testing::Args;
using ::testing::_;
using ::testing::Eq;

constexpr auto TEST_TRACK_NAME = "track";
constexpr auto TEST_PROCESSOR_NAME = "proc";
constexpr float TEST_SAMPLE_RATE = 44100;
constexpr Time TEST_MAX_INTERVAL = std::chrono::milliseconds(10);

// Custom Matcher to check the returned events
MATCHER_P5(ParameterChangeNotificationMatcherFull, proc_id, param_id, norm_val, dom_val, txt_val, "")
{
    auto typed_ev = static_cast<ParameterChangeNotificationEvent*>(arg);
    return arg->is_parameter_change_notification() &&
           typed_ev->processor_id() == proc_id &&
           typed_ev->parameter_id() == param_id &&
           typed_ev->normalized_value() == norm_val &&
           typed_ev->domain_value() == dom_val &&
           typed_ev->formatted_value() == txt_val;
}

MATCHER_P3(ParameterChangeNotificationMatcher, proc_id, param_id, norm_val, "")
{
    auto typed_ev = static_cast<ParameterChangeNotificationEvent*>(arg);
    return arg->is_parameter_change_notification() &&
           typed_ev->processor_id() == proc_id &&
           typed_ev->parameter_id() == param_id &&
           typed_ev->normalized_value() == norm_val;
}


class TestParameterManager : public ::testing::Test
{
protected:
    using ::testing::Test::SetUp; // Hide error of hidden overload of virtual function in clang when signatures differ but the name is the same
    TestParameterManager()
    {

    }

    void SetUp()
    {
        _test_processor = std::make_shared<gain_plugin::GainPlugin>(_host_control_mockup.make_host_control_mockup());
        _test_track = std::make_shared<Track>(_host_control_mockup.make_host_control_mockup(), 2, nullptr);
        ASSERT_EQ(ProcessorReturnCode::OK, _test_processor->init(TEST_SAMPLE_RATE));
        ASSERT_EQ(ProcessorReturnCode::OK, _test_track->init(TEST_SAMPLE_RATE));
        _test_processor->set_name(TEST_PROCESSOR_NAME);
        _test_track->set_name(TEST_TRACK_NAME);

        // Set up default returns for mock processor container
        ON_CALL(_mock_processor_container, processor(_test_track->id())).WillByDefault(Return(_test_track));
        ON_CALL(_mock_processor_container, processor(_test_processor->id())).WillByDefault(Return(_test_processor));

        _module_under_test.track_parameters(_test_processor->id());
        _module_under_test.track_parameters(_test_track->id());
    }

    ::testing::NiceMock<MockEventDispatcher> _mock_dispatcher;
    ::testing::NiceMock<MockProcessorContainer> _mock_processor_container;
    ParameterManager _module_under_test{TEST_MAX_INTERVAL, &_mock_processor_container};

    HostControlMockup _host_control_mockup;
    std::shared_ptr<Processor> _test_processor;
    std::shared_ptr<Track> _test_track;
};

TEST_F(TestParameterManager, TestEventCreation)
{
    EXPECT_CALL(_mock_dispatcher, process(ParameterChangeNotificationMatcherFull(3u, 4u, 0.5f, 5.0f, "5.0"))).Times(1);
    send_parameter_notification(3u, 4u, 0.5f, 5.0f, "5.0", &_mock_dispatcher);
}

TEST_F(TestParameterManager, TestParameterUpdates)
{
    _test_track->process_event(RtEvent::make_parameter_change_event(0, 0, 0, 0.7f));
    _test_processor->process_event(RtEvent::make_parameter_change_event(0, 0, 0, 0.6f));
    _module_under_test.mark_parameter_changed(_test_processor->id(), 0, TEST_MAX_INTERVAL);
    _module_under_test.mark_parameter_changed(_test_track->id(), 0, TEST_MAX_INTERVAL + Time(1));

    // Expect no notifications because time has not yet reached TEST_MAX_INTERVAL
    EXPECT_CALL(_mock_dispatcher, process(_)).Times(0);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, Time(1));

    // Expect 1 notification from test_processor
    EXPECT_CALL(_mock_dispatcher, process(ParameterChangeNotificationMatcher(_test_processor->id(), 0u, 0.6f))).Times(1);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, TEST_MAX_INTERVAL );

    // Expect the other notification from test_track
    EXPECT_CALL(_mock_dispatcher, process(ParameterChangeNotificationMatcher(_test_track->id(), 0u, 0.7f))).Times(1);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, TEST_MAX_INTERVAL + Time(3));

    // Expect no notifications as nothing has changed
    EXPECT_CALL(_mock_dispatcher, process(_)).Times(0);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, TEST_MAX_INTERVAL + Time(5));

    // Change a parameter, still expect no notification as one was sent too recently
    _test_track->process_event(RtEvent::make_parameter_change_event(_test_track->id(), 0, 0, 0.3f));
    _module_under_test.mark_parameter_changed(_test_track->id(), 0, 2 * TEST_MAX_INTERVAL);
    EXPECT_CALL(_mock_dispatcher, process(_)).Times(0);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, 2 * TEST_MAX_INTERVAL);

    // Expect 1 notification as we have advanced time sufficiently.
    EXPECT_CALL(_mock_dispatcher, process(ParameterChangeNotificationMatcher(_test_track->id(), 0u, 0.3f))).Times(1);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, 3 * TEST_MAX_INTERVAL);
}

TEST_F(TestParameterManager, TestProcessorUpdates)
{
    // Change every parameter value
    for (auto p : _test_track->all_parameters())
    {
        _test_track->process_event(RtEvent::make_parameter_change_event(_test_track->id(), 0, p->id(), 0.12345f));
    }
    _module_under_test.mark_processor_changed(_test_track->id(), TEST_MAX_INTERVAL);

    // Expect no notifications because time has not yet reached TEST_MAX_INTERVAL
    EXPECT_CALL(_mock_dispatcher, process(_)).Times(0);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, Time(0));

    // Expect 1 notification from every parameter of test_track
    EXPECT_CALL(_mock_dispatcher, process(_)).Times(_test_track->parameter_count());
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, 2 * TEST_MAX_INTERVAL);

    // Expect no notifications as nothing has changed
    EXPECT_CALL(_mock_dispatcher, process(_)).Times(0);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, TEST_MAX_INTERVAL + Time(5));
}


TEST_F(TestParameterManager, TestErrorHandling)
{
    // Notify processors that doesn't exist, should not crash, nor output anything
    _module_under_test.mark_processor_changed(12345, TEST_MAX_INTERVAL);
    _module_under_test.mark_parameter_changed(2345, 6789, TEST_MAX_INTERVAL);

    EXPECT_CALL(_mock_dispatcher, process(_)).Times(0);
    _module_under_test.output_parameter_notifications(&_mock_dispatcher, 2 * TEST_MAX_INTERVAL);

}
