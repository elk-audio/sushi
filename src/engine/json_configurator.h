/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Class to configure the audio engine using Json config files.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 *
 * @details This file contains class JsonConfigurator which provides public methods
 * to read configuration data from Json config files, validate its schema and configure
 * the engine with it. It can load tracks and midi connections from the config file.
 */

#ifndef SUSHI_CONFIG_FROM_JSON_H
#define SUSHI_CONFIG_FROM_JSON_H

#include <optional>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#include "rapidjson/document.h"
#pragma GCC diagnostic pop

#include "base_engine.h"
#include "engine/midi_dispatcher.h"
#include "control_frontends/osc_frontend.h"

namespace sushi {
namespace jsonconfig {

enum class JsonConfigReturnStatus
{
    OK,
    INVALID_CONFIGURATION,
    INVALID_TRACK_NAME,
    INVALID_PLUGIN_PATH,
    INVALID_PARAMETER,
    INVALID_PLUGIN_NAME,
    INVALID_MIDI_PORT,
    INVALID_FILE,
    NOT_DEFINED
};

enum class JsonSection
{
    HOST_CONFIG,
    TRACKS,
    PRE_TRACK,
    POST_TRACK,
    MIDI,
    OSC,
    CV_GATE,
    EVENTS,
    STATE
};

struct AudioConfig
{
    std::optional<int> cv_inputs;
    std::optional<int> cv_outputs;
    std::optional<int> midi_inputs;
    std::optional<int> midi_outputs;
    std::vector<std::tuple<int, int, bool>> rt_midi_input_mappings;
    std::vector<std::tuple<int, int, bool>> rt_midi_output_mappings;
};

class JsonConfigurator
{
public:
    JsonConfigurator(engine::BaseEngine* engine,
                     midi_dispatcher::MidiDispatcher* midi_dispatcher,
                     const engine::BaseProcessorContainer* processor_container,
                     const std::string& path) : _engine(engine),
                                                _midi_dispatcher(midi_dispatcher),
                                                _processor_container(processor_container),
                                                _document_path(path) {}

    ~JsonConfigurator() = default;

    /**
     * @brief Reads the json config, and returns all audio frontend configuration options
     *        that are not set on the audio engine directly.
     * @return A tuple of status and AudioConfig struct, AudioConfig is only valid if status is
     *         JsonConfigReturnStatus::OK
     */
    std::pair<JsonConfigReturnStatus, AudioConfig> load_audio_config();

    /**
     * @brief Reads the json config, and set the given host configuration options
     * @param path_to_file String which denotes the path of the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_host_config();

    /**
     * @brief Reads the json config, searches for valid track definitions and configures
     *        the engine with the specified tracks and master tracks
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_tracks();

    /**
     * @brief Reads the json config, searches for valid MIDI connections and
     *        MIDI CC Mapping definitions and configures the engine with the specified MIDI information.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_midi();

    /**
     * @brief Reads the json config, searches for valid OSC echo definitions and
     * configures the engine with the specified information.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_osc();

    /**
     * @brief Reads the json config, searches for valid control voltage and gate
     *        connection definitions and configures the engine with the specified routing.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_cv_gate();

    /**
     * @brief Reads the json config, searches for a valid "events" definition and
     *        queues them to the engines internal queue.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_events();

    /**
     * @brief Reads the json config, searches for a valid "events" definition and
     *        returns all parsed events as a list.
     * @return An std::vector with the parsed events which is only valid if the status
     *         returned is JsonConfigReturnStatus::OK
     */
    std::pair<JsonConfigReturnStatus, std::vector<Event*>> load_event_list();

    /**
     * @brief Reads the json config, searches for a valid "initial_state" definition
     *        and configures the processors with the values specified
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_initial_state();

    void set_osc_frontend(control_frontend::OSCFrontend* osc_frontend);

private:
    /**
     * @brief Helper function to retrieve a particular section of the json configuration
     * @param section Jsonsection to denote which section is to be validated.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    std::pair<JsonConfigReturnStatus, const rapidjson::Value&> _parse_section(JsonSection section);

    /**
     * @brief Uses Engine's API to create a single track with the specified number of channels and adds
     *        the respective plugins to the track if they are defined in the file. Used by load_tracks.
     * @param track_def rapidjson document object representing a single track and its details.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _make_track(const rapidjson::Value& track_def, engine::TrackType type);

    JsonConfigReturnStatus _connect_audio_to_track(const rapidjson::Value& track_def, const std::string& track_name, ObjectId track_id);

    JsonConfigReturnStatus _add_plugin(const rapidjson::Value& plugin_def, const std::string& track_name, ObjectId track_id);

    /**
     * @brief Helper function to extract the number of midi channels in the midi definition.
     * @param channels rapidjson document object containing the channel information parsed from the file.
     * @return The number of MIDI channels.
     */
    int _get_midi_channel(const rapidjson::Value& channels);

    /* Helper enum for more expressive code */
    enum EventParseMode : bool
    {
        IGNORE_TIMESTAMP = false,
        USE_TIMESTAMP = true,
    };
    /**
     * @brief Helper function to parse a single event
     * @param json_event A json value representing an event
     * @param with_timestamp If set to true, the timestamp from the json definition will be used
     *        if set to false, the event timestamp will be set for immediate processing
     * @return A pointer to an Event if successful, nullptr otherwise
     */
    Event* _parse_event(const rapidjson::Value& json_event, bool with_timestamp);

    /**
     * @brief function which validates the json data against the respective schema.
     * @param config rapidjson document object containing the json data parsed from the file
     * @param section JsonSection to denote which json section is to be validated.
     * @return true if json follows schema, false otherwise
     */
    static bool _validate_against_schema(rapidjson::Value& config, JsonSection section);

    JsonConfigReturnStatus _load_data();

    engine::BaseEngine* _engine;
    midi_dispatcher::MidiDispatcher* _midi_dispatcher;
    control_frontend::OSCFrontend* _osc_frontend {nullptr};
    const engine::BaseProcessorContainer* _processor_container;

    std::string _document_path;
    rapidjson::Document _json_data;
};

}/* namespace JSONCONFIG */
}/* namespace SUSHI */

#endif //SUSHI_CONFIG_FROM_JSON_H
