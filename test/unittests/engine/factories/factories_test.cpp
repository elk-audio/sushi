/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#include "sushi/offline_factory.h"
#include "sushi/standalone_factory.h"

#include "test_utils/test_utils.h"
#include "test_utils/portaudio_mockup.h"

constexpr int MOCK_CHANNEL_COUNT = 10;

#ifndef SUSHI_BUILD_WITH_PORTAUDIO

#include "audio_frontends/portaudio_frontend.h"

// Needed for mocking the frontend.
namespace sushi::internal::audio_frontend {

PortAudioFrontend::PortAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{
    _engine->set_audio_input_channels(MOCK_CHANNEL_COUNT);
    _engine->set_audio_output_channels(MOCK_CHANNEL_COUNT);
}

AudioFrontendStatus PortAudioFrontend::init(BaseAudioFrontendConfiguration*)
{
    return AudioFrontendStatus::OK;
}

}
#endif

#include "elk-warning-suppressor/warning_suppressor.hpp"

#ifndef OSCPACK_UNIT_TESTS
#include "third-party/oscpack/osc/OscPacketListener.h"
#include "third-party/oscpack/ip/UdpSocket.h"
#endif

#include "concrete_sushi.cpp"
#include "factories/base_factory.cpp"
#include "factories/offline_factory.cpp"
#include "factories/offline_factory_implementation.cpp"
#include "factories/reactive_factory.cpp"
#include "factories/reactive_factory_implementation.cpp"
#include "factories/standalone_factory.cpp"
#include "factories/standalone_factory_implementation.cpp"

#ifndef SUSHI_BUILD_WITH_APPLE_COREAUDIO
#include "audio_frontends/apple_coreaudio_frontend.cpp"
#endif

using namespace std::chrono_literals;

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::_;

using namespace sushi;
using namespace sushi::internal;

//////////////////////////////////////////////////////
// ReactiveFactory
//////////////////////////////////////////////////////

class ReactiveFactoryTest : public ::testing::Test
{
protected:
    ReactiveFactoryTest() = default;

    void SetUp() override
    {
        options.config_filename = "NONE";
        options.config_source = ConfigurationSource::NONE;

        _path = test_utils::get_data_dir_path();
    }

    SushiOptions options;

    ReactiveFactoryImplementation _reactive_factory;

    std::string _path;
};

TEST_F(ReactiveFactoryTest, TestReactiveFactoryWithDefaultConfig)
{
    options.config_filename = "NONE";
    options.config_source = ConfigurationSource::NONE;

    auto [sushi, status] = _reactive_factory.new_instance(options);

    ASSERT_NE(sushi.get(), nullptr);

    auto sushi_cast = static_cast<ConcreteSushi*>(sushi.get());

    ASSERT_NE(sushi_cast, nullptr);

    sushi::internal::ConcreteSushiAccessor _accessor(*sushi_cast);

    EXPECT_NE(_accessor.osc_frontend().get(), nullptr);
    EXPECT_NE(_accessor.engine().get(), nullptr);
    EXPECT_NE(_accessor.midi_dispatcher().get(), nullptr);
    EXPECT_NE(_accessor.midi_frontend().get(), nullptr);
    EXPECT_NE(_accessor.audio_frontend().get(), nullptr);
    EXPECT_NE(_accessor.frontend_config().get(), nullptr);
    EXPECT_NE(_accessor.engine_controller().get(), nullptr);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    EXPECT_NE(sushi_cast->_rpc_server.get(), nullptr);
#endif

    auto rt_controller = _reactive_factory.rt_controller();

    EXPECT_NE(rt_controller.get(), nullptr);
}

TEST_F(ReactiveFactoryTest, TestPassiveFactoryWithConfigFile)
{
    // Currently, the Passive frontend supports only stereo I/O, so a simpler config is used.
    // JsonConfigurator is already extensively tested elsewhere anyway.
    _path.append("config_single_stereo.json");

    options.config_filename = _path;
    options.config_source = ConfigurationSource::FILE;

    auto [sushi, status] = _reactive_factory.new_instance(options);

    ASSERT_NE(sushi.get(), nullptr);

    auto sushi_cast = static_cast<ConcreteSushi*>(sushi.get());

    ASSERT_NE(sushi_cast, nullptr);

    sushi::internal::ConcreteSushiAccessor _accessor(*sushi_cast);

    EXPECT_NE(_accessor.osc_frontend().get(), nullptr);
    EXPECT_NE(_accessor.engine().get(), nullptr);
    EXPECT_NE(_accessor.midi_dispatcher().get(), nullptr);
    EXPECT_NE(_accessor.midi_frontend().get(), nullptr);
    EXPECT_NE(_accessor.audio_frontend().get(), nullptr);
    EXPECT_NE(_accessor.frontend_config().get(), nullptr);
    EXPECT_NE(_accessor.engine_controller().get(), nullptr);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    EXPECT_NE(sushi_cast->_rpc_server.get(), nullptr);
#endif

    auto rt_controller = _reactive_factory.rt_controller();

    EXPECT_NE(rt_controller.get(), nullptr);
}

//////////////////////////////////////////////////////
// OfflineFactory
//////////////////////////////////////////////////////

class OfflineFactoryTest : public ::testing::Test
{
protected:
    OfflineFactoryTest() = default;

    void SetUp() override
    {
        options.config_filename = "NONE";
        options.config_source = ConfigurationSource::NONE;

        _path = test_utils::get_data_dir_path();
    }

    SushiOptions options;

    OfflineFactory _offline_factory;

    std::string _path;
};

TEST_F(OfflineFactoryTest, TestOfflineFactoryWithDefaultConfig)
{
    options.config_filename = "NONE";
    options.config_source = ConfigurationSource::NONE;

    auto [sushi, status] = _offline_factory.new_instance(options);

    ASSERT_NE(sushi.get(), nullptr);

    auto sushi_cast = static_cast<ConcreteSushi*>(sushi.get());

    ASSERT_NE(sushi_cast, nullptr);

    sushi::internal::ConcreteSushiAccessor _accessor(*sushi_cast);

    // OSC Frontend instantiation will not be implemented for the offline factory.
    EXPECT_EQ(_accessor.osc_frontend().get(), nullptr);

    EXPECT_NE(_accessor.engine().get(), nullptr);
    EXPECT_NE(_accessor.midi_dispatcher().get(), nullptr);
    EXPECT_NE(_accessor.midi_frontend().get(), nullptr);
    EXPECT_NE(_accessor.audio_frontend().get(), nullptr);
    EXPECT_NE(_accessor.frontend_config().get(), nullptr);
    EXPECT_NE(_accessor.engine_controller().get(), nullptr);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    EXPECT_NE(sushi_cast->_rpc_server.get(), nullptr);
#endif
}

TEST_F(OfflineFactoryTest, TestOfflineFactoryWithConfigFile)
{
    _path.append("config.json");

    options.config_filename = _path;
    options.config_source = ConfigurationSource::FILE;

    auto [sushi, status] = _offline_factory.new_instance(options);

    ASSERT_NE(sushi.get(), nullptr);

    auto sushi_cast = static_cast<ConcreteSushi*>(sushi.get());

    ASSERT_NE(sushi_cast, nullptr);

    sushi::internal::ConcreteSushiAccessor _accessor(*sushi_cast);

    // OSC Frontend instantiation will not be implemented for the offline factory.
    EXPECT_EQ(_accessor.osc_frontend().get(), nullptr);

    EXPECT_NE(_accessor.engine().get(), nullptr);
    EXPECT_NE(_accessor.midi_dispatcher().get(), nullptr);
    EXPECT_NE(_accessor.midi_frontend().get(), nullptr);
    EXPECT_NE(_accessor.audio_frontend().get(), nullptr);
    EXPECT_NE(_accessor.frontend_config().get(), nullptr);
    EXPECT_NE(_accessor.engine_controller().get(), nullptr);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    EXPECT_NE(sushi_cast->_rpc_server.get(), nullptr);
#endif
}


//////////////////////////////////////////////////////
// StandaloneFactory
//////////////////////////////////////////////////////

class StandaloneFactoryTest : public ::testing::Test
{
protected:
    StandaloneFactoryTest() = default;

    void SetUp() override
    {
        mockPortAudio = std::make_unique<NiceMock<MockPortAudio>>();

        PaError init_value = PaErrorCode::paNoError;
        EXPECT_CALL(*mockPortAudio, Pa_Initialize).WillRepeatedly(Return(init_value));
        EXPECT_CALL(*mockPortAudio, Pa_GetDeviceCount).WillRepeatedly(Return(1));
        EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo).WillRepeatedly(Return(&device_info));
        EXPECT_CALL(*mockPortAudio, Pa_GetStreamInfo).WillRepeatedly(Return(&stream_info));
        EXPECT_CALL(*mockPortAudio, Pa_OpenStream).WillRepeatedly(Return(init_value));

        options.config_filename = "NONE";
        options.config_source = ConfigurationSource::NONE;

        device_info.maxInputChannels = MOCK_CHANNEL_COUNT;
        device_info.maxOutputChannels = MOCK_CHANNEL_COUNT;

        _path = test_utils::get_data_dir_path();
    }

    void TearDown() override
    {
        mockPortAudio.reset();
    }

    PaDeviceInfo device_info;
    PaStreamInfo stream_info;

    SushiOptions options;

    StandaloneFactory _standalone_factory;

    std::string _path;
};

TEST_F(StandaloneFactoryTest, TestStandaloneFactoryWithDefaultConfig)
{
    auto expected_name = "a_device";

#ifdef SUSHI_BUILD_WITH_PORTAUDIO
    PaDeviceInfo device_info;
    device_info.maxInputChannels = 1;
    device_info.maxOutputChannels = 1;
    device_info.name = expected_name;

    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo)
#ifdef __APPLE__
    .WillOnce(Return(&device_info))
#endif
    .WillOnce(Return(&device_info))
    .WillOnce(Return(&device_info));
#endif

    options.config_filename = "NONE";
    options.config_source = ConfigurationSource::NONE;
    options.frontend_type = FrontendType::PORTAUDIO;
    options.device_name = expected_name;

    auto [sushi, status] = _standalone_factory.new_instance(options);

    ASSERT_NE(sushi.get(), nullptr);

    auto sushi_cast = static_cast<ConcreteSushi*>(sushi.get());

    ASSERT_NE(sushi_cast, nullptr);

    sushi::internal::ConcreteSushiAccessor _accessor(*sushi_cast);

    EXPECT_NE(_accessor.osc_frontend().get(), nullptr);
    EXPECT_NE(_accessor.engine().get(), nullptr);
    EXPECT_NE(_accessor.midi_dispatcher().get(), nullptr);
    EXPECT_NE(_accessor.midi_frontend().get(), nullptr);
    EXPECT_NE(_accessor.audio_frontend().get(), nullptr);
    EXPECT_NE(_accessor.frontend_config().get(), nullptr);
    EXPECT_NE(_accessor.engine_controller().get(), nullptr);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    EXPECT_NE(sushi_cast->_rpc_server.get(), nullptr);
#endif
}

TEST_F(StandaloneFactoryTest, TestStandaloneFactoryWithConfigFile)
{
    auto expected_name = "a_device";

#ifdef SUSHI_BUILD_WITH_PORTAUDIO
    PaDeviceInfo device_info;
    device_info.maxInputChannels = 2;
    device_info.maxOutputChannels = 2;
    device_info.name = expected_name;

    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo)
#ifdef __APPLE__
    .WillOnce(Return(&device_info))
#endif
    .WillOnce(Return(&device_info))
    .WillOnce(Return(&device_info));
#endif

    _path.append("config_single_stereo.json");

    options.config_filename = _path;
    options.config_source = ConfigurationSource::FILE;
    options.frontend_type = FrontendType::PORTAUDIO;
    options.device_name = expected_name;

    auto [sushi, status] = _standalone_factory.new_instance(options);

    ASSERT_NE(sushi.get(), nullptr);

    auto sushi_cast = static_cast<ConcreteSushi*>(sushi.get());

    ASSERT_NE(sushi_cast, nullptr);

    sushi::internal::ConcreteSushiAccessor _accessor(*sushi_cast);

    EXPECT_NE(_accessor.osc_frontend().get(), nullptr);
    EXPECT_NE(_accessor.engine().get(), nullptr);
    EXPECT_NE(_accessor.midi_dispatcher().get(), nullptr);
    EXPECT_NE(_accessor.midi_frontend().get(), nullptr);
    EXPECT_NE(_accessor.audio_frontend().get(), nullptr);
    EXPECT_NE(_accessor.frontend_config().get(), nullptr);
    EXPECT_NE(_accessor.engine_controller().get(), nullptr);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    EXPECT_NE(sushi_cast->_rpc_server.get(), nullptr);
#endif
}
