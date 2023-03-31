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

#include "sushi/control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"

namespace sushi::internal::engine::controller_impl {

class AudioRoutingController : public control::AudioRoutingController
{
public:
    explicit AudioRoutingController(BaseEngine* engine) : _engine(engine),
                                                          _event_dispatcher(engine->event_dispatcher()) {}

    ~AudioRoutingController() override = default;

    std::vector<control::AudioConnection> get_all_input_connections() const override;

    std::vector<control::AudioConnection> get_all_output_connections() const override;

    std::vector<control::AudioConnection> get_input_connections_for_track(int track_id) const override;

    std::vector<control::AudioConnection> get_output_connections_for_track(int track_id) const override;

    control::ControlStatus connect_input_channel_to_track(int track_id, int track_channel, int input_channel) override;

    control::ControlStatus connect_output_channel_to_track(int track_id, int track_channel, int output_channel) override;

    control::ControlStatus disconnect_input(int track_id, int track_channel, int input_channel) override;

    control::ControlStatus disconnect_output(int track_id, int track_channel, int output_channel) override;

    control::ControlStatus disconnect_all_inputs_from_track(int track_id) override;

    control::ControlStatus disconnect_all_outputs_from_track(int track_id) override;

private:
    BaseEngine* _engine;
    dispatcher::BaseEventDispatcher* _event_dispatcher;
};

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_AUDIO_ROUTING_CONTROLLER_H
