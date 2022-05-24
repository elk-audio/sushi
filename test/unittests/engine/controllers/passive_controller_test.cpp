#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#define private public
#define protected public

#include "engine/controller/passive_controller.cpp"

// TODO: Mock these too!
#include "audio_frontends/passive_frontend.cpp"
#include "control_frontends/passive_midi_frontend.cpp"

#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"
#include "test_utils/mock_sushi.h"

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::_;

using namespace sushi;

constexpr float TEST_SAMPLE_RATE = 44100;

class PassiveControllerTestFrontend : public ::testing::Test
{
protected:
    PassiveControllerTestFrontend() {}

    void SetUp()
    {
        _passive_controller = std::make_unique<PassiveController>(std::make_unique<MockSushi>());
        _mock_sushi = static_cast<MockSushi*>(_passive_controller->_sushi.get());
    }

    void TearDown() {}

    std::unique_ptr<PassiveController> _passive_controller;

    MockSushi* _mock_sushi;

    EngineMockup _mock_engine {TEST_SAMPLE_RATE};
};

TEST_F(PassiveControllerTestFrontend, TestInitialization)
{
    SushiOptions options;

    EXPECT_CALL(*_mock_sushi, init)
        .Times(1)
        .WillRepeatedly(Return(InitStatus::OK));

    EXPECT_CALL(*_mock_sushi, audio_frontend)
        .Times(1);

    EXPECT_CALL(*_mock_sushi, midi_frontend)
        .Times(1);

    EXPECT_CALL(*_mock_sushi, audio_engine)
        .Times(1)
        .WillOnce(Return(&_mock_engine));

    EXPECT_CALL(*_mock_sushi, start)
        .Times(1);

    auto status = _passive_controller->init(options);

    EXPECT_EQ(InitStatus::OK, status);
}