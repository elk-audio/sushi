/*
* Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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

#include "test_utils/test_utils.h"

#define private public
#define protected public

#include "sushi.cpp"
#include "factories/factory_base.cpp"
#include "factories/passive_factory.cpp"


using namespace std::chrono_literals;

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::_;

using namespace sushi;

constexpr float TEST_SAMPLE_RATE = 44100;

class PassiveFactoryTest : public ::testing::Test
{
protected:
    PassiveFactoryTest() {}

    void SetUp()
    {
        options.config_filename = "NONE";
        options.use_input_config_file = false;

        _path = test_utils::get_data_dir_path();
    }

    void TearDown() {}

    SushiOptions options;

    PassiveFactory _passive_factory;

    std::string _path;
};

TEST_F(PassiveFactoryTest, TestPassiveFactoryWithDefaultConfig)
{
    options.config_filename = "NONE";
    options.use_input_config_file = false;

    auto sushi = _passive_factory.run(options);

    ASSERT_NE(sushi.get(), nullptr);

    auto sushi_cast = static_cast<Sushi*>(sushi.get());

    ASSERT_NE(sushi_cast, nullptr);

    // TODO: OSC Frontend instantiation is not implemented yet for the passive factory.
    EXPECT_EQ(sushi_cast->_osc_frontend.get(), nullptr);

    EXPECT_NE(sushi_cast->_engine.get(), nullptr);
    EXPECT_NE(sushi_cast->_midi_dispatcher.get(), nullptr);
    EXPECT_NE(sushi_cast->_midi_frontend.get(), nullptr);
    EXPECT_NE(sushi_cast->_audio_frontend.get(), nullptr);
    EXPECT_NE(sushi_cast->_frontend_config.get(), nullptr);
    EXPECT_NE(sushi_cast->_engine_controller.get(), nullptr);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    EXPECT_NE(sushi_cast->_rpc_server.get(), nullptr);
#endif

    auto rt_controller = _passive_factory.rt_controller();

    EXPECT_NE(rt_controller.get(), nullptr);
}

TEST_F(PassiveFactoryTest, TestPassiveFactoryWithConfigFile)
{
    // Currently, the Passive frontend supports only stereo I/O, so a simpler config is used.
    // JsonConfigurator is already extensively tested elsewhere anyway.
    _path.append("config_single_stereo.json");

    options.config_filename = _path;
    options.use_input_config_file = true;

    auto sushi = _passive_factory.run(options);

    ASSERT_NE(sushi.get(), nullptr);

    auto sushi_cast = static_cast<Sushi*>(sushi.get());

    ASSERT_NE(sushi_cast, nullptr);

    // TODO: OSC Frontend instantiation is not implemented yet for the passive factory.
    EXPECT_EQ(sushi_cast->_osc_frontend.get(), nullptr);

    EXPECT_NE(sushi_cast->_engine.get(), nullptr);
    EXPECT_NE(sushi_cast->_midi_dispatcher.get(), nullptr);
    EXPECT_NE(sushi_cast->_midi_frontend.get(), nullptr);
    EXPECT_NE(sushi_cast->_audio_frontend.get(), nullptr);
    EXPECT_NE(sushi_cast->_frontend_config.get(), nullptr);
    EXPECT_NE(sushi_cast->_engine_controller.get(), nullptr);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    EXPECT_NE(sushi_cast->_rpc_server.get(), nullptr);
#endif

    auto rt_controller = _passive_factory.rt_controller();

    EXPECT_NE(rt_controller.get(), nullptr);
}
