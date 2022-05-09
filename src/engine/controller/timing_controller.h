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

#ifndef SUSHI_TIMING_CONTROLLER_H
#define SUSHI_TIMING_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "library/base_performance_timer.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class TimingController : public ext::TimingController
{
public:
    explicit TimingController(BaseEngine* engine);

    ~TimingController() override = default;

    bool get_timing_statistics_enabled() const override;

    void set_timing_statistics_enabled(bool enabled) override;

    std::pair<ext::ControlStatus, ext::CpuTimings> get_engine_timings() const override;

    std::pair<ext::ControlStatus, ext::CpuTimings> get_track_timings(int track_id) const override;

    std::pair<ext::ControlStatus, ext::CpuTimings> get_processor_timings(int processor_id) const override;

    ext::ControlStatus reset_all_timings() override;

    ext::ControlStatus reset_track_timings(int track_id) override;

    ext::ControlStatus reset_processor_timings(int processor_id) override;

private:
    std::pair<ext::ControlStatus, ext::CpuTimings> _get_timings(int node) const;

    performance::BasePerformanceTimer*  _performance_timer;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_TIMING_CONTROLLER_H
