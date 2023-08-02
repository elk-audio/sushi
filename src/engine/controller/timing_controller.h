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

#ifndef SUSHI_TIMING_CONTROLLER_H
#define SUSHI_TIMING_CONTROLLER_H

#include "sushi/control_interface.h"

#include "engine/base_engine.h"
#include "library/base_performance_timer.h"

namespace sushi::internal::engine::controller_impl {

class TimingController : public control::TimingController
{
public:
    explicit TimingController(BaseEngine* engine);

    ~TimingController() override = default;

    bool get_timing_statistics_enabled() const override;

    void set_timing_statistics_enabled(bool enabled) override;

    std::pair<control::ControlStatus, control::CpuTimings> get_engine_timings() const override;

    std::pair<control::ControlStatus, control::CpuTimings> get_track_timings(int track_id) const override;

    std::pair<control::ControlStatus, control::CpuTimings> get_processor_timings(int processor_id) const override;

    control::ControlStatus reset_all_timings() override;

    control::ControlStatus reset_track_timings(int track_id) override;

    control::ControlStatus reset_processor_timings(int processor_id) override;

private:
    std::pair<control::ControlStatus, control::CpuTimings> _get_timings(int node) const;

    performance::BasePerformanceTimer*  _performance_timer;
};

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_TIMING_CONTROLLER_H
