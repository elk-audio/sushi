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

#ifndef SUSHI_AUDIO_ROUTING_CONTROLLER_H
#define SUSHI_AUDIO_ROUTING_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class AudioRoutingController : public ext::AudioRoutingController
{
public:
    explicit AudioRoutingController(BaseEngine* engine) : _engine(engine),
                                                          _event_dispatcher(engine->event_dispatcher()) {}

    ~AudioRoutingController() override = default;

    std::vector<ext::AudioConnection> get_all_input_connections() const override;

    std::vector<ext::AudioConnection> get_all_output_connections() const override;

    std::vector<ext::AudioConnection> get_input_connections_for_track(int track_id) const override;

    std::vector<ext::AudioConnection> get_output_connections_for_track(int track_id) const override;

    ext::ControlStatus connect_input_channel_to_track(int track_id, int track_channel, int input_channel) override;

    ext::ControlStatus connect_output_channel_to_track(int track_id, int track_channel, int output_channel) override;

    ext::ControlStatus disconnect_input(int track_id, int track_channel, int input_channel) override;

    ext::ControlStatus disconnect_output(int track_id, int track_channel, int output_channel) override;

    ext::ControlStatus disconnect_all_inputs_from_track(int track_id) override;

    ext::ControlStatus disconnect_all_outputs_from_track(int track_id) override;

private:
    BaseEngine* _engine;
    dispatcher::BaseEventDispatcher* _event_dispatcher;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_AUDIO_ROUTING_CONTROLLER_H
