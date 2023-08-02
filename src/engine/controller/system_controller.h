/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_SYSTEM_CONTROLLER_H
#define SUSHI_SYSTEM_CONTROLLER_H

#include "sushi/compile_time_settings.h"
#include "sushi/control_interface.h"

namespace sushi::internal::engine::controller_impl {

class SystemController : public control::SystemController
{
public:
    SystemController(int inputs, int outputs);

    ~SystemController() override = default;

    std::string get_sushi_version() const override;

    control::SushiBuildInfo get_sushi_build_info() const override;

    int get_input_audio_channel_count() const override;

    int get_output_audio_channel_count() const override;

private:
    std::vector<std::string> _build_options;
    control::SushiBuildInfo _build_info;

    const int _audio_inputs{0};
    const int _audio_outputs{0};
};

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_SYSTEM_CONTROLLER_H
