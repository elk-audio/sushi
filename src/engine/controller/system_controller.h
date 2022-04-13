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

#ifndef SUSHI_SYSTEM_CONTROLLER_H
#define SUSHI_SYSTEM_CONTROLLER_H

#include <compile_time_settings.h>
#include "control_interface.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class SystemController : public ext::SystemController
{
public:
    SystemController(int inputs, int outputs);

    ~SystemController() override = default;

    std::string get_sushi_version() const override;

    ext::SushiBuildInfo get_sushi_build_info() const override;

    int get_input_audio_channel_count() const override;

    int get_output_audio_channel_count() const override;

private:
    std::vector<std::string> _build_options;
    ext::SushiBuildInfo _build_info;

    const int _audio_inputs{0};
    const int _audio_outputs{0};
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_SYSTEM_CONTROLLER_H
