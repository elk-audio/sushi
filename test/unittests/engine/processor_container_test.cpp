#include <thread>

#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"

#define private public
#define protected public

#include "engine/processor_container.cpp"

constexpr float SAMPLE_RATE = 44000;
using namespace sushi;
using namespace sushi::engine;

class TestProcessorContainer : public ::testing::Test
{
protected:
    TestProcessorContainer() {}

    HostControlMockup  _hc;
    ProcessorContainer _module_under_test;
};

TEST_F(TestProcessorContainer, TestAddingAndRemoving)
{
    auto proc_1 = std::make_shared<DummyProcessor>(_hc.make_host_control_mockup(SAMPLE_RATE));
    proc_1->set_name("one");
    auto proc_2 = std::make_shared<DummyProcessor>(_hc.make_host_control_mockup(SAMPLE_RATE));
    proc_2->set_name("two");
    auto id_1 = proc_1->id();
    auto id_2 = proc_2->id();
    ASSERT_TRUE(_module_under_test.add_processor(proc_1));
    ASSERT_TRUE(_module_under_test.add_processor(proc_2));

    // Assert false when adding proc_2 again
    ASSERT_FALSE(_module_under_test.add_processor(proc_2));

    // Access these processors
    ASSERT_TRUE(_module_under_test.processor_exists(id_1));
    ASSERT_TRUE(_module_under_test.processor_exists("two"));
    ASSERT_EQ("one", _module_under_test.processor("one")->name());
    ASSERT_EQ(id_2, _module_under_test.processor(id_2)->id());
    ASSERT_EQ(proc_2, _module_under_test.mutable_processor(id_2));
    ASSERT_EQ(proc_1, _module_under_test.mutable_processor(id_1));
    ASSERT_EQ(2u, _module_under_test.all_processors().size());

    // Access non-existing processors
    ASSERT_FALSE(_module_under_test.processor_exists(ObjectId(123)));
    ASSERT_FALSE(_module_under_test.processor_exists("three"));
    ASSERT_EQ(nullptr, _module_under_test.processor("four"));
    ASSERT_EQ(nullptr, _module_under_test.processor(ObjectId(234)));

    // Remove processors
    ASSERT_TRUE(_module_under_test.remove_processor(id_1));
    ASSERT_TRUE(_module_under_test.remove_processor(id_2));
    ASSERT_FALSE(_module_under_test.remove_processor(id_1));

    ASSERT_FALSE(_module_under_test.processor_exists(id_1));
    ASSERT_FALSE(_module_under_test.processor_exists("two"));
    ASSERT_EQ(nullptr, _module_under_test.mutable_processor(id_2));
    ASSERT_EQ(nullptr, _module_under_test.mutable_processor(id_1));
}

TEST_F(TestProcessorContainer, TestTrackManagement)
{
    auto proc_1 = std::make_shared<DummyProcessor>(_hc.make_host_control_mockup(SAMPLE_RATE));
    proc_1->set_name("one");
    auto proc_2 = std::make_shared<DummyProcessor>(_hc.make_host_control_mockup(SAMPLE_RATE));
    proc_2->set_name("two");
    auto track = std::make_shared<Track>(_hc.make_host_control_mockup(SAMPLE_RATE), 2, nullptr);
    track->set_name("track");

    ASSERT_TRUE(_module_under_test.add_processor(proc_1));
    ASSERT_TRUE(_module_under_test.add_processor(proc_2));
    ASSERT_TRUE(_module_under_test.add_processor(track));

    ASSERT_TRUE(_module_under_test.add_track(track));
    ASSERT_FALSE(_module_under_test.add_track(track));

    ASSERT_TRUE(_module_under_test.add_to_track(proc_1, track->id(), std::nullopt));
    ASSERT_TRUE(_module_under_test.add_to_track(proc_2, track->id(), proc_1->id()));

    ASSERT_TRUE(_module_under_test.processor_exists(track->id()));
    ASSERT_EQ(track, _module_under_test.track(track->id()));
    ASSERT_EQ(track, _module_under_test.track("track"));
    ASSERT_EQ(nullptr, _module_under_test.track("two"));

    auto procs = _module_under_test.processors_on_track(track->id());
    ASSERT_EQ(2u, procs.size());
    ASSERT_EQ("two", procs[0]->name());
    ASSERT_EQ("one", procs[1]->name());

    ASSERT_TRUE(_module_under_test.remove_from_track(proc_2->id(), track->id()));
    procs = _module_under_test.processors_on_track(track->id());
    ASSERT_EQ(1u, procs.size());
    ASSERT_EQ("one", procs[0]->name());

    ASSERT_TRUE(_module_under_test.remove_from_track(proc_1->id(), track->id()));
    ASSERT_TRUE(_module_under_test.remove_processor(proc_1->id()));
    ASSERT_TRUE(_module_under_test.remove_processor(proc_2->id()));
    ASSERT_TRUE(_module_under_test.remove_track(track->id()));
    ASSERT_TRUE(_module_under_test.remove_processor(track->id()));

    ASSERT_TRUE(_module_under_test.all_tracks().empty());
    ASSERT_FALSE(_module_under_test.processor_exists("track"));
    ASSERT_FALSE(_module_under_test.processor_exists("one"));
    ASSERT_FALSE(_module_under_test.processor_exists("two"));
}
