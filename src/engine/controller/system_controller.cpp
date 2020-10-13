/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Implementation of external control interface for sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "system_controller.h"

#include "generated/version.h"

namespace sushi {
namespace engine {
namespace controller_impl {

SystemController::SystemController(BaseEngine* engine) : _engine(engine)
{
    // TODO Ilias: Populate interface version!
    _interface_version = "1.2.3. POPULATE THIS PLACEHOLDER";

    _sushi_version = std::to_string(SUSHI__VERSION_MAJ) + "." +
                     std::to_string(SUSHI__VERSION_MIN) + "." +
                     std::to_string(SUSHI__VERSION_REV);

#ifdef SUSHI_BUILD_WITH_VST2
    _build_options.push_back("vst2");
#endif
#ifdef SUSHI_BUILD_WITH_VST3
    _build_options.push_back("vst3");
#endif
#ifdef SUSHI_BUILD_WITH_LV2
    _build_options.push_back("lv2");
#endif
#ifdef SUSHI_BUILD_WITH_JACK
    _build_options.push_back("jack");
#endif
#ifdef SUSHI_BUILD_WITH_XENOMAI
    _build_options.push_back("xenomai");
#endif
#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _build_options.push_back("rpc control");
#endif
#ifdef SUSHI_BUILD_WITH_ABLETON_LINK
    _build_options.push_back("ableton link");
#endif

    // TODO Ilias: What is this? Where do I get it? Isn't it already in _sushi_version fetched separately?
    _build_info.version = _sushi_version;

    _build_info.build_options = _build_options;
    _build_info.audio_buffer_size = AUDIO_CHUNK_SIZE;
    _build_info.commit_hash = SUSHI_GIT_COMMIT_HASH;
    _build_info.build_date = SUSHI_BUILD_TIMESTAMP;

    _audio_inputs = engine->audio_input_channels();
    _audio_outputs = engine->audio_output_channels();
}

std::string SystemController::get_interface_version() const
{
    // I'm guessing this is the gRPC interface version.
    return _interface_version;
}

std::string SystemController::get_sushi_version() const
{
    // from main void print_version_and_build_info()
    return _sushi_version;
}

ext::SushiBuildInfo SystemController::get_sushi_build_info() const
{
    // main, constexpr std::array SUSHI_ENABLED_BUILD_OPTIONS
    return _build_info;
}

int SystemController::get_input_audio_channel_count() const
{
    // Probably ENGINE_CHANNELS = 8;
    return _audio_inputs;
}

int SystemController::get_output_audio_channel_count() const
{
    // int _audio_inputs{0};
    // int _audio_outputs{0};
    // in BaseEngine
    return _audio_outputs;
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi