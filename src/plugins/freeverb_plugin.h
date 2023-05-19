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
 * @brief Wrapper for Freeverb plugin
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef FREEVERB_PLUGIN_H
#define FREEVERB_PLUGIN_H

#include <revmodel.hpp>

#include "library/internal_plugin.h"

namespace sushi {
namespace freeverb_plugin {

class FreeverbPlugin : public InternalPlugin, public UidHelper<FreeverbPlugin>
{
public:
    explicit FreeverbPlugin(HostControl hostControl);

    ~FreeverbPlugin() override = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    BypassManager _bypass_manager{false, std::chrono::milliseconds(100)};
    float _sample_rate{0};

    BoolParameterValue*  _freeze;
    FloatParameterValue* _dry;
    FloatParameterValue* _wet;
    FloatParameterValue* _room_size;
    FloatParameterValue* _width;
    FloatParameterValue* _damp;

    std::unique_ptr<revmodel> _reverb_model;
};

}// namespace freeverb_plugin
}// namespace sushi
#endif // FREEVERB_PLUGIN_H
