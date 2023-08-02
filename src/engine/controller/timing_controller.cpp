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

/**
 * @brief Implementation of external control interface for sushi.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "timing_controller.h"
#include "elklog/static_logger.h"

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi::internal::engine::controller_impl {

TimingController::TimingController(engine::BaseEngine* engine) : _performance_timer(engine->performance_timer())
{}

inline control::CpuTimings to_external(performance::ProcessTimings& internal)
{
    return {internal.avg_case, internal.min_case, internal.max_case};
}

bool TimingController::get_timing_statistics_enabled() const
{
    ELKLOG_LOG_DEBUG("get_timing_statistics_enabled called");
    return _performance_timer->enabled();
}

void TimingController::set_timing_statistics_enabled(bool enabled)
{
    ELKLOG_LOG_DEBUG("set_timing_statistics_enabled called with {}", enabled);
    // TODO - do this by events instead.
    _performance_timer->enable(enabled);
}

std::pair<control::ControlStatus, control::CpuTimings> TimingController::get_engine_timings() const
{
    ELKLOG_LOG_DEBUG("get_engine_timings called, returning ");
    return _get_timings(engine::ENGINE_TIMING_ID);
}

std::pair<control::ControlStatus, control::CpuTimings> TimingController::get_track_timings(int track_id) const
{
    ELKLOG_LOG_DEBUG("get_track_timings called, returning ");
    return _get_timings(track_id);
}

std::pair<control::ControlStatus, control::CpuTimings> TimingController::get_processor_timings(int processor_id) const
{
    ELKLOG_LOG_DEBUG("get_processor_timings called, returning ");
    return _get_timings(processor_id);
}

control::ControlStatus TimingController::reset_all_timings()
{
    ELKLOG_LOG_DEBUG("reset_all_timings called, returning ");
    _performance_timer->clear_all_timings();
    return control::ControlStatus::OK;
}

control::ControlStatus TimingController::reset_track_timings(int track_id)
{
    ELKLOG_LOG_DEBUG("reset_track_timings called, returning ");
    auto success =_performance_timer->clear_timings_for_node(track_id);
    return success? control::ControlStatus::OK : control::ControlStatus::NOT_FOUND;
}

control::ControlStatus TimingController::reset_processor_timings(int processor_id)
{
    ELKLOG_LOG_DEBUG("reset_processor_timings called, returning ");
    return reset_track_timings(processor_id);
}

std::pair<control::ControlStatus, control::CpuTimings> TimingController::_get_timings(int node) const
{
    if (_performance_timer->enabled())
    {
        auto timings = _performance_timer->timings_for_node(node);
        if (timings.has_value())
        {
            return {control::ControlStatus::OK, to_external(timings.value())};
        }
        return {control::ControlStatus::NOT_FOUND, {0,0,0}};
    }
    return {control::ControlStatus::UNSUPPORTED_OPERATION, {0,0,0}};
}

} // end namespace sushi::internal::engine::controller_impl