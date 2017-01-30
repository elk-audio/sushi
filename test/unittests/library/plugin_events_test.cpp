#include "gtest/gtest.h"
#define private public

#include "library/plugin_events.h"

using namespace sushi;

TEST (TestKeyboardEvent, TestEventCreation)
{
    KeyboardEvent event(EventType::NOTE_ON, 25, 60, 1.0f);
    EXPECT_EQ(EventType::NOTE_ON, event.type());
    EXPECT_EQ(25, event.sample_offset());
    EXPECT_EQ(60, event.note());
    EXPECT_FLOAT_EQ(1.0f, event.velocity());
}


