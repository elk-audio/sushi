/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Utility functions for dumping plugins' parameter info
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "json_utils.h"
#include "library/parameter_dump.h"
#include "control_frontends/osc_utils.h"

namespace sushi {

rapidjson::Document generate_processor_parameter_document(sushi::ext::SushiControl* engine_controller)
{
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    rapidjson::Value processors(rapidjson::kArrayType);
    auto graph_controller = engine_controller->audio_graph_controller();
    auto param_controller = engine_controller->parameter_controller();

    for(auto& track : graph_controller->get_all_tracks())
    {
        for(auto& processor : graph_controller->get_track_processors(track.id).second)
        {
            rapidjson::Value processor_obj(rapidjson::kObjectType);
            processor_obj.AddMember(rapidjson::Value("name", allocator).Move(),
                                    rapidjson::Value(processor.name.c_str(), allocator).Move(), allocator);

            processor_obj.AddMember(rapidjson::Value("label", allocator).Move(),
                                    rapidjson::Value(processor.label.c_str(), allocator).Move(), allocator);

            processor_obj.AddMember(rapidjson::Value("processor_id", allocator).Move(),
                                    rapidjson::Value(processor.id).Move(), allocator);

            processor_obj.AddMember(rapidjson::Value("parent_track_id", allocator).Move(),
                                    rapidjson::Value(track.id).Move(), allocator);

            rapidjson::Value parameters(rapidjson::kArrayType);

            for(auto& parameter : param_controller->get_processor_parameters(processor.id).second)
            {
                rapidjson::Value parameter_obj(rapidjson::kObjectType);
                parameter_obj.AddMember(rapidjson::Value("name", allocator).Move(),
                                        rapidjson::Value(parameter.name.c_str(), allocator).Move(), allocator);

                parameter_obj.AddMember(rapidjson::Value("label", allocator).Move(),
                                        rapidjson::Value(parameter.label.c_str(), allocator).Move(), allocator);

                std::string osc_path("/parameter/" + osc::make_safe_path(processor.name) + "/" +
                                     osc::make_safe_path(parameter.name));
                parameter_obj.AddMember(rapidjson::Value("osc_path", allocator).Move(),
                                        rapidjson::Value(osc_path.c_str(), allocator).Move(), allocator);

                parameter_obj.AddMember(rapidjson::Value("id", allocator).Move(),
                                        rapidjson::Value(parameter.id).Move(), allocator);

                parameters.PushBack(parameter_obj.Move(),allocator);
            }
            processor_obj.AddMember(rapidjson::Value("parameters", allocator).Move(), parameters.Move(), allocator);
            processors.PushBack(processor_obj.Move(), allocator);
        }
    }

    document.AddMember(rapidjson::Value("plugins", allocator),processors.Move(), allocator);

    return document;
}

} // end namespace sushi

