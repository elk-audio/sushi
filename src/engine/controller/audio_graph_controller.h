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

#ifndef SUSHI_AUDIO_GRAPH_CONTROLLER_H
#define SUSHI_AUDIO_GRAPH_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"
#include "controller_common.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class AudioGraphController : public ext::AudioGraphController
{
public:
    explicit AudioGraphController(BaseEngine* engine);

    ~AudioGraphController() override = default;

    std::vector<ext::ProcessorInfo> get_all_processors() const override;

    std::vector<ext::TrackInfo> get_all_tracks() const override;

    std::pair<ext::ControlStatus, int> get_track_id(const std::string& track_name) const override;

    std::pair<ext::ControlStatus, ext::TrackInfo> get_track_info(int track_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::ProcessorInfo>> get_track_processors(int track_id) const override;

    std::pair<ext::ControlStatus, int> get_processor_id(const std::string& processor_name) const override;

    std::pair<ext::ControlStatus, ext::ProcessorInfo> get_processor_info(int processor_id) const override;

    std::pair<ext::ControlStatus, bool> get_processor_bypass_state(int processor_id) const override;

    std::pair<ext::ControlStatus, ext::ProcessorState> get_processor_state(int processor_id) const override;

    ext::ControlStatus set_processor_bypass_state(int processor_id, bool bypass_enabled) override;

    ext::ControlStatus set_processor_state(int processor_id, const ext::ProcessorState& state) override;

    ext::ControlStatus create_track(const std::string& name, int channels) override;

    ext::ControlStatus create_multibus_track(const std::string& name, int buses) override;

    ext::ControlStatus create_pre_track(const std::string& name) override;

    ext::ControlStatus create_post_track(const std::string& name) override;

    ext::ControlStatus move_processor_on_track(int processor_id,
                                               int source_track_id,
                                               int dest_track_id,
                                               std::optional<int> before_processor) override;

    ext::ControlStatus create_processor_on_track(const std::string& name,
                                                 const std::string& uid,
                                                 const std::string& file,
                                                 ext::PluginType type,
                                                 int track_id,
                                                 std::optional<int> before_processor_id) override;

    ext::ControlStatus delete_processor_from_track(int processor_id, int track_id) override;

    ext::ControlStatus delete_track(int track_id) override;


private:
    std::vector<int> _get_processor_ids(int track_id) const;

    engine::BaseEngine*                     _engine;
    dispatcher::BaseEventDispatcher*        _event_dispatcher;
    const engine::BaseProcessorContainer*   _processors;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_AUDIO_GRAPH_CONTROLLER_H
