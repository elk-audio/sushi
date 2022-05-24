#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#undef private

#include "engine/controller/passive_controller.cpp"

// TODO: Mock these too!
#include "audio_frontends/passive_frontend.cpp"
#include "control_frontends/passive_midi_frontend.cpp"

#include "test_utils/control_mockup.h"
#include "test_utils/mock_sushi.h"

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::_;

using namespace sushi;

class PassiveControllerTestFrontend : public ::testing::Test
{
protected:
    PassiveControllerTestFrontend() {}

    void SetUp()
    {
        _passive_controller = std::make_unique<PassiveController>(std::make_unique<MockSushi>());
    }

    void TearDown() {}

    std::unique_ptr<PassiveController> _passive_controller;

    std::unique_ptr<AbstractSushi> _mock_sushi;
};

TEST_F(PassiveControllerTestFrontend, TestAThing)
{

}