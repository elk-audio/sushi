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

#include "timing_controller.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

TimingController::TimingController(sushi::engine::BaseEngine* engine) : _performance_timer(engine->performance_timer())
{}

inline ext::CpuTimings to_external(sushi::performance::ProcessTimings& internal)
{
    return {internal.avg_case, internal.min_case, internal.max_case};
}

bool TimingController::get_timing_statistics_enabled() const
{
    SUSHI_LOG_DEBUG("get_timing_statistics_enabled called");
    return _performance_timer->enabled();
}

void TimingController::set_timing_statistics_enabled(bool enabled)
{
    SUSHI_LOG_DEBUG("set_timing_statistics_enabled called with {}", enabled);
    // TODO - do this by events instead.
    _performance_timer->enable(enabled);
}

std::pair<ext::ControlStatus, ext::CpuTimings> TimingController::get_engine_timings() const
{
    SUSHI_LOG_DEBUG("get_engine_timings called, returning ");
    return _get_timings(engine::ENGINE_TIMING_ID);
}

std::pair<ext::ControlStatus, ext::CpuTimings> TimingController::get_track_timings(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_timings called, returning ");
    return _get_timings(track_id);
}

std::pair<ext::ControlStatus, ext::CpuTimings> TimingController::get_processor_timings(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_timings called, returning ");
    return _get_timings(processor_id);
}

ext::ControlStatus TimingController::reset_all_timings()
{
    SUSHI_LOG_DEBUG("reset_all_timings called, returning ");
    _performance_timer->clear_all_timings();
    return ext::ControlStatus::OK;
}

ext::ControlStatus TimingController::reset_track_timings(int track_id)
{
    SUSHI_LOG_DEBUG("reset_track_timings called, returning ");
    auto success =_performance_timer->clear_timings_for_node(track_id);
    return success? ext::ControlStatus::OK : ext::ControlStatus::NOT_FOUND;
}

ext::ControlStatus TimingController::reset_processor_timings(int processor_id)
{
    SUSHI_LOG_DEBUG("reset_processor_timings called, returning ");
    return reset_track_timings(processor_id);
}

std::pair<ext::ControlStatus, ext::CpuTimings> TimingController::_get_timings(int node) const
{
    if (_performance_timer->enabled())
    {
        auto timings = _performance_timer->timings_for_node(node);
        if (timings.has_value())
        {
            return {ext::ControlStatus::OK, to_external(timings.value())};
        }
        return {ext::ControlStatus::NOT_FOUND, {0,0,0}};
    }
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, {0,0,0}};
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi