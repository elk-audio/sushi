/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 */

#include <fstream>

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/schema.h"

#include "logging.h"
#include "json_configurator.h"

namespace sushi {
namespace jsonconfig {

using namespace engine;
using namespace midi_dispatcher;

SUSHI_GET_LOGGER_WITH_MODULE_NAME("jsonconfig");

constexpr int ERROR_DISPLAY_CHARS = 50;

std::pair<JsonConfigReturnStatus, AudioConfig> JsonConfigurator::load_audio_config()
{
    AudioConfig audio_config;
    auto [status, host_config] = _parse_section(JsonSection::HOST_CONFIG);
    if(status != JsonConfigReturnStatus::OK)
    {
        return {status, audio_config};
    }

    if (host_config.HasMember("cv_inputs"))
    {
        audio_config.cv_inputs = host_config["cv_inputs"].GetInt();
    }
    if (host_config.HasMember("cv_outputs"))
    {
        audio_config.cv_outputs = host_config["cv_outputs"].GetInt();
    }
    if (host_config.HasMember("midi_inputs"))
    {
        audio_config.midi_inputs = host_config["midi_inputs"].GetInt();
    }
    if (host_config.HasMember("midi_outputs"))
    {
        audio_config.midi_outputs = host_config["midi_outputs"].GetInt();
    }

    return {JsonConfigReturnStatus::OK, audio_config};
}

JsonConfigReturnStatus JsonConfigurator::load_host_config()
{
    auto [status, host_config] = _parse_section(JsonSection::HOST_CONFIG);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }

    float sample_rate = host_config["samplerate"].GetFloat();
    SUSHI_LOG_INFO("Setting engine sample rate to {}", sample_rate);
    _engine->set_sample_rate(sample_rate);

    if (host_config.HasMember("tempo"))
    {
        float tempo = host_config["tempo"].GetFloat();
        SUSHI_LOG_INFO("Setting engine tempo to {}", sample_rate);
        _engine->set_tempo(tempo);
    }

    if (host_config.HasMember("time_signature"))
    {
        const auto& sig = host_config["time_signature"].GetObject();
        int numerator = sig["numerator"].GetInt();
        int denominator = sig["denominator"].GetInt();

        SUSHI_LOG_INFO("Setting engine time signature to {}/{}", numerator, denominator);
        _engine->set_time_signature({numerator, denominator});
    }

    if (host_config.HasMember("playing_mode"))
    {
        PlayingMode mode;
        if (host_config["playing_mode"] == "stopped")
        {
            mode = PlayingMode::STOPPED;
        }
        else
        {
            mode = PlayingMode::PLAYING;
        }
        SUSHI_LOG_INFO("Setting engine playing mode to {}", mode == PlayingMode::PLAYING ? "playing " : "stopped");
        _engine->set_transport_mode(mode);
    }

    if (host_config.HasMember("tempo_sync"))
    {
        SyncMode mode;
        if (host_config["tempo_sync"] == "ableton_link")
        {
            mode = SyncMode::ABLETON_LINK;
        }
        else if (host_config["tempo_sync"] == "midi")
        {
            mode = SyncMode::MIDI;
        }
        else if (host_config["tempo_sync"] == "gate")
        {
            mode = SyncMode::GATE_INPUT;
        }
        else
        {
            mode = SyncMode::INTERNAL;
        }
        SUSHI_LOG_INFO("Setting engine tempo sync mode to {}", mode == SyncMode::ABLETON_LINK? "Ableton Link" : (
                                                               mode == SyncMode::MIDI ? "external Midi" : (
                                                               mode == SyncMode::GATE_INPUT? "Gate input" : "internal")));
        _engine->set_tempo_sync_mode(mode);
    }

    if (host_config.HasMember("audio_clip_detection"))
    {
        const auto& clip_det = host_config["audio_clip_detection"].GetObject();
        if (clip_det.HasMember("inputs"))
        {
            _engine->enable_input_clip_detection(clip_det["inputs"].GetBool());
            SUSHI_LOG_INFO("Setting engine input clip detection {}", clip_det["inputs"].GetBool() ? "enabled" : "disabled");
        }
        if (clip_det.HasMember("outputs"))
        {
            _engine->enable_output_clip_detection(clip_det["outputs"].GetBool());
            SUSHI_LOG_INFO("Setting engine output clip detection {}", clip_det["outputs"].GetBool() ? "enabled" : "disabled");
        }
    }

    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_tracks()
{
    auto [status, tracks] = _parse_section(JsonSection::TRACKS);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }

    for (auto& track : tracks.GetArray())
    {
        status = _make_track(track);
        if (status != JsonConfigReturnStatus::OK)
        {
            return status;
        }
    }
    SUSHI_LOG_INFO("Successfully configured engine with tracks in JSON config file \"{}\"", _document_path);
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_midi()
{
    auto [status, midi] = _parse_section(JsonSection::MIDI);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    if(midi.HasMember("track_connections"))
    {
        for (const auto& con : midi["track_connections"].GetArray())
        {
            bool raw_midi = con["raw_midi"].GetBool();

            auto track_name = con["track"].GetString();
            const auto track = _processor_container->track(track_name);
            if (track == nullptr)
            {
                SUSHI_LOG_ERROR("Invalid plugin track \"{}\" for midi "
                                "track connection in Json config file.", con["track"].GetString());
                return JsonConfigReturnStatus::INVALID_TRACK_NAME;
            }

            MidiDispatcherStatus res;
            if (raw_midi)
            {
                res = _midi_dispatcher->connect_raw_midi_to_track(con["port"].GetInt(),
                                                                  track->id(),
                                                                  _get_midi_channel(con["channel"]));
            }
            else
            {
                res = _midi_dispatcher->connect_kb_to_track(con["port"].GetInt(),
                                                            track->id(),
                                                            _get_midi_channel(con["channel"]));
            }
            if (res != MidiDispatcherStatus::OK)
            {
                if(res == MidiDispatcherStatus::INVALID_MIDI_INPUT)
                {
                    SUSHI_LOG_ERROR("Invalid port \"{}\" specified specified for midi "
                                           "channel connections in Json Config file.", con["port"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
            }
        }
    }

    if(midi.HasMember("track_out_connections"))
    {
        for (const auto& con : midi["track_out_connections"].GetArray())
        {
            auto track_name = con["track"].GetString();
            const auto track = _processor_container->track(track_name);
            if (track == nullptr)
            {
                SUSHI_LOG_ERROR("Invalid plugin track \"{}\" for midi "
                                "track connection in Json config file.", con["track"].GetString());
                return JsonConfigReturnStatus::INVALID_TRACK_NAME;
            }

            auto res = _midi_dispatcher->connect_track_to_output(con["port"].GetInt(),
                                                                 track->id(),
                                                                 _get_midi_channel(con["channel"]));
            if (res != MidiDispatcherStatus::OK)
            {
                if(res == MidiDispatcherStatus::INVALID_MIDI_OUTPUT)
                {
                    SUSHI_LOG_ERROR("Invalid port \"{}\" specified for midi "
                                           "channel connections in Json Config file.", con["port"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
                else if(res == MidiDispatcherStatus::INVAlID_CHANNEL)
                {
                    SUSHI_LOG_ERROR("Invalid channel \"{}\" specified for midi "
                                    "channel connections in Json Config file.", con["channel"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
            }
        }
    }

    if(midi.HasMember("program_change_connections"))
    {
        for (const auto& con : midi["program_change_connections"].GetArray())
        {
            auto processor_name = con["plugin"].GetString();
            const auto processor = _processor_container->processor(processor_name);
            if (processor == nullptr)
            {
                SUSHI_LOG_ERROR("Invalid plugin \"{}\" for MIDI program change "
                                "connection in Json config file.", processor_name);
                return JsonConfigReturnStatus::INVALID_PLUGIN_NAME;
            }

            auto res = _midi_dispatcher->connect_pc_to_processor(con["port"].GetInt(),
                                                                 processor->id(),
                                                                 _get_midi_channel(con["channel"]));
            if (res != MidiDispatcherStatus::OK)
            {
                if(res == MidiDispatcherStatus::INVALID_MIDI_INPUT)
                {
                    SUSHI_LOG_ERROR("Invalid port \"{}\" specified specified for MIDI program change "
                                   "channel connections in Json Config file.", con["port"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
            }
        }
    }

    if(midi.HasMember("cc_mappings"))
    {
        for (const auto& cc_map : midi["cc_mappings"].GetArray())
        {
            bool is_relative = false;
            if (cc_map.HasMember("mode"))
            {
                auto cc_mode = std::string(cc_map["mode"].GetString());
                is_relative = (cc_mode.compare("relative") == 0);
            }

            auto processor_name = cc_map["plugin_name"].GetString();

            const auto processor = _processor_container->processor(processor_name);
            if (processor == nullptr)
            {
                SUSHI_LOG_ERROR("Invalid plugin \"{}\" for MIDI program change "
                                "connection in Json config file.", processor_name);
                return JsonConfigReturnStatus::INVALID_PLUGIN_NAME;
            }

            auto parameter_name = cc_map["parameter_name"].GetString();
            const auto parameter = processor->parameter_from_name(parameter_name);
            if (parameter == nullptr)
            {
                return JsonConfigReturnStatus::INVALID_PARAMETER;
            }

            auto res = _midi_dispatcher->connect_cc_to_parameter(cc_map["port"].GetInt(),
                                                                 processor->id(),
                                                                 parameter->id(),
                                                                 cc_map["cc_number"].GetInt(),
                                                                 cc_map["min_range"].GetFloat(),
                                                                 cc_map["max_range"].GetFloat(),
                                                                 is_relative,
                                                                 _get_midi_channel(cc_map["channel"]));
            if (res != MidiDispatcherStatus::OK)
            {
                if(res == MidiDispatcherStatus::INVALID_MIDI_INPUT)
                {
                    SUSHI_LOG_ERROR("Invalid port \"{}\" specified "
                                           "for midi cc mappings in Json Config file.", cc_map["port"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
                if(res == MidiDispatcherStatus::INVALID_PROCESSOR)
                {
                    SUSHI_LOG_ERROR("Invalid plugin name \"{}\" specified "
                                           "for midi cc mappings in Json Config file.", cc_map["plugin_name"].GetString());
                    return JsonConfigReturnStatus::INVALID_TRACK_NAME;
                }
                SUSHI_LOG_ERROR("Invalid parameter name \"{}\" specified for plugin \"{}\" for midi cc mappings.",
                                                                            cc_map["parameter_name"].GetString(),
                                                                            cc_map["plugin_name"].GetString());
                return JsonConfigReturnStatus::INVALID_PARAMETER;
            }
        }
    }

    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_osc()
{
    auto [status, osc_config] = _parse_section(JsonSection::OSC);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }

    if (osc_config.HasMember("enable_all_outputs"))
    {
        bool enabled = osc_config["enable_all_outputs"].GetBool();
        if(enabled)
        {
            _osc_frontend->connect_from_all_parameters();
        }
        else // While the current default is off, it may not always be, so why not have this wired up.
        {
            _osc_frontend->disconnect_from_all_parameters();
        }
        SUSHI_LOG_INFO("Setting engine input clip detection {}", enabled ? "enabled" : "disabled");
    }

    if (osc_config.HasMember("osc_outputs"))
    {
        for (const auto& osc_out : osc_config["osc_outputs"].GetArray())
        {
            auto processor_name = osc_out["processor"].GetString();

            if (osc_out.HasMember("parameter"))
            {
                bool res = _osc_frontend->connect_from_parameter(processor_name,
                                                                osc_out["parameter"].GetString());
                if (res != true)
                {
                    SUSHI_LOG_ERROR("Failed to enable osc output of parameter {} on processor {}",
                                    osc_out["parameter"].GetString(),
                                    processor_name);
                }
            }
            else
            {
                auto processor = _processor_container->processor(processor_name);
                bool res = false;

                if(processor.get() != nullptr)
                {
                    res = _osc_frontend->connect_from_processor_parameters(processor_name, processor->id());
                }

                if (res != true)
                {
                    SUSHI_LOG_ERROR("Failed to enable osc output of parameter {} on processor {}",
                                    osc_out["parameter"].GetString(),
                                    processor_name);
                }
            }
        }
    }
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_cv_gate()
{
    auto [status, cv_config] = _parse_section(JsonSection::CV_GATE);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    if (cv_config.HasMember("cv_inputs"))
    {
        for (const auto& cv_in : cv_config["cv_inputs"].GetArray())
        {
            auto res = _engine->connect_cv_to_parameter(cv_in["processor"].GetString(),
                                                        cv_in["parameter"].GetString(),
                                                        cv_in["cv"].GetInt());
            if (res != EngineReturnStatus::OK)
            {
                SUSHI_LOG_ERROR("Failed to connect cv input {} to parameter {} on processor {}",
                               cv_in["cv"].GetInt(),
                               cv_in["parameter"].GetString(),
                               cv_in["processor"].GetString());
            }
        }
    }
    if (cv_config.HasMember("cv_outputs"))
    {
        for (const auto& cv_out : cv_config["cv_outputs"].GetArray())
        {
            auto res = _engine->connect_cv_from_parameter(cv_out["processor"].GetString(),
                                                          cv_out["parameter"].GetString(),
                                                          cv_out["cv"].GetInt());
            if (res != EngineReturnStatus::OK)
            {
                SUSHI_LOG_ERROR("Failed to connect cv output {} to parameter {} on processor {}",
                               cv_out["cv"].GetInt(),
                               cv_out["parameter"].GetString(),
                               cv_out["processor"].GetString());
            }
        }
    }
    if (cv_config.HasMember("gate_inputs"))
    {
        for (const auto& gate_in : cv_config["gate_inputs"].GetArray())
        {
            if (gate_in["mode"] == "sync")
            {
                auto res = _engine->connect_gate_to_sync(gate_in["gate"].GetInt(),
                                                         gate_in["ppq_ticks"].GetInt());
                if (res != EngineReturnStatus::OK)
                {
                    SUSHI_LOG_ERROR("Failed to set gate {} as sync input", gate_in["gate"].GetInt());
                }

            }
            else if (gate_in["mode"] == "note_event")
            {
                auto res = _engine->connect_gate_to_processor(gate_in["processor"].GetString(),
                                                              gate_in["gate"].GetInt(),
                                                              gate_in["note_no"].GetInt(),
                                                              gate_in["channel"].GetInt());
                if (res != EngineReturnStatus::OK)
                {
                    SUSHI_LOG_ERROR("Failed to connect gate {} to processor {}",
                                   gate_in["gate"].GetInt(),
                                   gate_in["processor"].GetInt());
                }
            }
        }
    }
    if (cv_config.HasMember("gate_outputs"))
    {
        for (const auto& gate_out : cv_config["gate_outputs"].GetArray())
        {
            if (gate_out["mode"] == "sync")
            {
                auto res = _engine->connect_sync_to_gate(gate_out["gate"].GetInt(),
                                                         gate_out["ppq_ticks"].GetInt());
                if (res != EngineReturnStatus::OK)
                {
                    SUSHI_LOG_ERROR("Failed to set gate {} as sync output", gate_out["gate"].GetInt());
                }

            }
            else if (gate_out["mode"] == "note_event")
            {
                auto res = _engine->connect_gate_from_processor(gate_out["processor"].GetString(),
                                                                gate_out["gate"].GetInt(),
                                                                gate_out["note_no"].GetInt(),
                                                                gate_out["channel"].GetInt());
                if (res != EngineReturnStatus::OK)
                {
                    SUSHI_LOG_ERROR("Failed to connect gate {} from processor {}",
                                   gate_out["gate"].GetInt(),
                                   gate_out["processor"].GetInt());
                }
            }
        }
    }
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_events()
{
    auto [status, events] = _parse_section(JsonSection::EVENTS);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    auto dispatcher = _engine->event_dispatcher();
    for (auto& json_event : events.GetArray())
    {
        if (Event* e = _parse_event(json_event, IGNORE_TIMESTAMP); e != nullptr)
            dispatcher->post_event(e);
    }
    return JsonConfigReturnStatus::OK;
}

std::pair<JsonConfigReturnStatus, std::vector<Event*>>
JsonConfigurator::load_event_list()
{
    std::vector<Event*> events;
    auto [status, json_events] = _parse_section(JsonSection::EVENTS);
    if(status != JsonConfigReturnStatus::OK)
    {
        return std::make_pair(status, events);
    }
    for (auto& json_event : json_events.GetArray())
    {
        if (Event* e = _parse_event(json_event, USE_TIMESTAMP); e != nullptr)
            events.push_back(e);
    }
    return std::make_pair(JsonConfigReturnStatus::OK, events);
}

void JsonConfigurator::set_osc_frontend(control_frontend::OSCFrontend* osc_frontend)
{
    _osc_frontend = osc_frontend;
}

std::pair<JsonConfigReturnStatus, const rapidjson::Value&> JsonConfigurator::_parse_section(JsonSection section)
{
    if (_json_data.IsObject() == false)
    {
        auto res = _load_data();
        if (res != JsonConfigReturnStatus::OK)
        {
            return {res, _json_data};
        }
    }
    if(_validate_against_schema(_json_data, section) == false)
    {
        SUSHI_LOG_ERROR("Config file {} does not follow schema: {}", _document_path, (int)section);
        return {JsonConfigReturnStatus::INVALID_CONFIGURATION, _json_data};
    }

    switch(section)
    {
        case JsonSection::HOST_CONFIG:
            if(_json_data.HasMember("host_config") == false)
            {
                SUSHI_LOG_INFO("Config file does not have any Host Config definitions");
                return {JsonConfigReturnStatus::NO_MIDI_DEFINITIONS, _json_data};
            }
            return {JsonConfigReturnStatus::OK, _json_data["host_config"]};

        case JsonSection::TRACKS:
            if(_json_data.HasMember("tracks") == false)
            {
                SUSHI_LOG_INFO("Config file does not have any Track definitions");
                return {JsonConfigReturnStatus::NO_MIDI_DEFINITIONS, _json_data};
            }
            return {JsonConfigReturnStatus::OK, _json_data["tracks"]};

        case JsonSection::MIDI:
            if(_json_data.HasMember("midi") == false)
            {
                SUSHI_LOG_INFO("Config file does not have MIDI definitions");
                return {JsonConfigReturnStatus::NO_MIDI_DEFINITIONS, _json_data};
            }
            return {JsonConfigReturnStatus::OK, _json_data["midi"]};

        case JsonSection::OSC:
            if(_json_data.HasMember("osc") == false)
            {
                SUSHI_LOG_INFO("Config file does not have OSC mapping definitions");
                return {JsonConfigReturnStatus::NO_OSC_DEFINITIONS, _json_data};
            }
            return {JsonConfigReturnStatus::OK, _json_data["osc"]};

        case JsonSection::CV_GATE:
            if(_json_data.HasMember("cv_control") == false)
            {
                SUSHI_LOG_INFO("Config file does not have CV/Gate definitions");
                return {JsonConfigReturnStatus::NO_CV_GATE_DEFINITIONS, _json_data};
            }
            return {JsonConfigReturnStatus::OK, _json_data["cv_control"]};

        case JsonSection::EVENTS:
            if(_json_data.HasMember("events") == false)
            {
                SUSHI_LOG_INFO("Config file does not have any Event definitions");
                return {JsonConfigReturnStatus::NO_EVENTS_DEFINITIONS, _json_data};
            }
            return {JsonConfigReturnStatus::OK, _json_data["events"]};

        default:
            return {JsonConfigReturnStatus::INVALID_CONFIGURATION, _json_data};
    }
}

JsonConfigReturnStatus JsonConfigurator::_make_track(const rapidjson::Value &track_def)
{
    auto name = track_def["name"].GetString();
    EngineReturnStatus status = EngineReturnStatus::ERROR;
    ObjectId track_id;
    if (track_def["mode"] == "mono")
    {
        std::tie(status, track_id) = _engine->create_track(name, 1);
    }
    else if (track_def["mode"] == "stereo")
    {
        std::tie(status, track_id) = _engine->create_track(name, 2);
    }
    else if (track_def["mode"] == "multibus")
    {
        if (track_def.HasMember("input_busses") && track_def.HasMember("output_busses"))
        {
            std::tie(status, track_id) = _engine->create_multibus_track(name,
                                                                        track_def["input_busses"].GetInt(),
                                                                        track_def["output_busses"].GetInt());
        }
    }
    else
    {
        return JsonConfigReturnStatus::INVALID_CONFIGURATION;
    }

    if(status == EngineReturnStatus::INVALID_PLUGIN || status == EngineReturnStatus::INVALID_PROCESSOR)
    {
        SUSHI_LOG_ERROR("Track {} in JSON config file duplicate or invalid name", name);
        return JsonConfigReturnStatus::INVALID_TRACK_NAME;
    }
    if(status != EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Track Name {} failed to create", name);
        return JsonConfigReturnStatus::INVALID_CONFIGURATION;
    }

    SUSHI_LOG_DEBUG("Successfully added track \"{}\" to the engine", name);

    for(const auto& con : track_def["inputs"].GetArray())
    {
        if (con.HasMember("engine_bus"))
        {
            status = _engine->connect_audio_input_bus(con["engine_bus"].GetInt(),
                                                      con["track_bus"].GetInt(),
                                                      track_id);
        }
        else
        {
            status = _engine->connect_audio_input_channel(con["engine_channel"].GetInt(),
                                                          con["track_channel"].GetInt(),
                                                          track_id);
        }
        if(status != EngineReturnStatus::OK)
        {
            SUSHI_LOG_ERROR("Error connecting input bus to track \"{}\", error {}", name, static_cast<int>(status));
            return JsonConfigReturnStatus::INVALID_CONFIGURATION;
        }
    }

    for(const auto& con : track_def["outputs"].GetArray())
    {
        if (con.HasMember("engine_bus"))
        {
            status = _engine->connect_audio_output_bus(con["engine_bus"].GetInt(),
                                                       con["track_bus"].GetInt(),
                                                       track_id);
        }
        else
        {
            status = _engine->connect_audio_output_channel(con["engine_channel"].GetInt(),
                                                           con["track_channel"].GetInt(),
                                                           track_id);

        }
        if(status != EngineReturnStatus::OK)
        {
            SUSHI_LOG_ERROR("Error connection track \"{}\" to output bus, error {}", name, static_cast<int>(status));
            return JsonConfigReturnStatus::INVALID_CONFIGURATION;
        }
    }

    for(const auto& def : track_def["plugins"].GetArray())
    {
        std::string plugin_uid;
        std::string plugin_path;
        std::string plugin_name = def["name"].GetString();
        PluginType plugin_type;
        std::string type = def["type"].GetString();
        if(type == "internal")
        {
            plugin_type = PluginType::INTERNAL;
            plugin_uid = def["uid"].GetString();
        }
        else if(type == "vst2x")
        {
            plugin_type = PluginType::VST2X;
            plugin_path = def["path"].GetString();
        }
        else if(type == "vst3x")
        {
            plugin_uid = def["uid"].GetString();
            plugin_path = def["path"].GetString();
            plugin_type = PluginType::VST3X;
        }
        else // Anything else should have been caught by the validation step before this
        {
            plugin_type = PluginType::LV2;
            plugin_path = def["uri"].GetString();
        }

        auto [status, plugin_id] = _engine->load_plugin(plugin_uid, plugin_name, plugin_path, plugin_type);
        if(status != EngineReturnStatus::OK)
        {
            if(status == EngineReturnStatus::INVALID_PLUGIN_UID)
            {
                SUSHI_LOG_ERROR("Invalid plugin uid {} in JSON config file", plugin_uid);
                return JsonConfigReturnStatus::INVALID_PLUGIN_PATH;
            }
            SUSHI_LOG_ERROR("Plugin Name {} in JSON config file already exists in engine", plugin_name);
            return JsonConfigReturnStatus::INVALID_PLUGIN_NAME;
        }
        status = _engine->add_plugin_to_track(plugin_id, track_id);
        if (status != EngineReturnStatus::OK)
        {
            return JsonConfigReturnStatus::INVALID_CONFIGURATION;
        }
        SUSHI_LOG_DEBUG("Successfully added Plugin \"{}\" to"
                               " Chain \"{}\"", plugin_name, name);
    }

    SUSHI_LOG_DEBUG("Successfully added Track {} to the engine", name);
    return JsonConfigReturnStatus::OK;
}

int JsonConfigurator::_get_midi_channel(const rapidjson::Value& channels)
{
    if (channels.IsString())
    {
        return midi::MidiChannel::OMNI;
    }
    return channels.GetInt();
}

Event* JsonConfigurator::_parse_event(const rapidjson::Value& json_event, bool with_timestamp)
{
    Time timestamp = with_timestamp? std::chrono::microseconds(
            static_cast<int>(std::round(json_event["time"].GetDouble() * 1'000'000))): IMMEDIATE_PROCESS;

    const rapidjson::Value& data = json_event["data"];
    auto processor = _engine->processor_container()->processor(data["plugin_name"].GetString());
    if (processor == nullptr)
    {
        SUSHI_LOG_WARNING("Unrecognised plugin: \"{}\"", data["plugin_name"].GetString());
        return nullptr;
    }
    if (json_event["type"] == "parameter_change")
    {
        auto parameter = processor->parameter_from_name(data["parameter_name"].GetString());
        if (parameter == nullptr)
        {
            SUSHI_LOG_WARNING("Unrecognised parameter: {}", data["parameter_name"].GetString());
            return nullptr;
        }
        return new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                        processor->id(),
                                        parameter->id(),
                                        data["value"].GetFloat(),
                                        timestamp);
    }
    if (json_event["type"] == "property_change")
    {
        auto parameter = processor->parameter_from_name(data["parameter_name"].GetString());
        if (parameter == nullptr)        {
            SUSHI_LOG_WARNING("Unrecognised property: {}", data["property_name"].GetString());
            return nullptr;
        }
        return new StringPropertyChangeEvent(processor->id(),
                                             parameter->id(),
                                             data["value"].GetString(),
                                             timestamp);
    }
    if (json_event["type"] == "note_on")
    {
        return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON,
                                 processor->id(),
                                 0, // channel
                                 data["note"].GetUint(),
                                 data["velocity"].GetFloat(),
                                 timestamp);
    }
    if (json_event["type"] == "note_off")
    {
        return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF,
                                 processor->id(),
                                 0, // channel
                                 data["note"].GetUint(),
                                 data["velocity"].GetFloat(),
                                 timestamp);
    }
    return nullptr;
}

bool JsonConfigurator::_validate_against_schema(rapidjson::Value& config, JsonSection section)
{
    const char* schema_char_array = nullptr;
    switch(section)
    {
        case JsonSection::HOST_CONFIG:
            schema_char_array =
                #include "json_schemas/host_config_schema.json"
                                                              ;
            break;

        case JsonSection::TRACKS:
            schema_char_array =
                #include "json_schemas/tracks_schema.json"
                                                        ;
            break;

        case JsonSection::MIDI:
            schema_char_array =
                #include "json_schemas/midi_schema.json"
                                                       ;
            break;

        case JsonSection::OSC:
            schema_char_array =
                #include "json_schemas/osc_schema.json"
                                                      ;
            break;

        case JsonSection::CV_GATE:
            schema_char_array =
                #include "json_schemas/cv_gate_schema.json"
                                                         ;
            break;

        case JsonSection::EVENTS:
            schema_char_array =
                #include "json_schemas/events_schema.json"
                                                        ;
    }

    rapidjson::Document schema;
    schema.Parse(schema_char_array);
    rapidjson::SchemaDocument schema_document(schema);
    rapidjson::SchemaValidator schema_validator(schema_document);

    // Validate Schema
    if (!config.Accept(schema_validator))
    {
        rapidjson::Pointer invalid_config_pointer = schema_validator.GetInvalidDocumentPointer();
        rapidjson::StringBuffer string_buffer;
        invalid_config_pointer.Stringify(string_buffer);
        std::string error_node = string_buffer.GetString();
        if(error_node.empty() == false)
        {
            SUSHI_LOG_ERROR("Schema validation failure at {}", error_node);
        }
        return false;
    }
    return true;
}

JsonConfigReturnStatus JsonConfigurator::_load_data()
{
    std::ifstream config_file(_document_path);
    if(!config_file.good())
    {
        SUSHI_LOG_ERROR("Invalid file passed to JsonConfigurator {}", _document_path);
        return JsonConfigReturnStatus::INVALID_FILE;
    }
    //iterate through every char in file and store in the string
    std::string config_file_contents((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
    _json_data.Parse(config_file_contents.c_str());
    if(_json_data.HasParseError())
    {
        [[maybe_unused]] int err_offset = _json_data.GetErrorOffset();
        SUSHI_LOG_ERROR("Error parsing JSON config file: {} @ pos {}: \"{}\"",
                       rapidjson::GetParseError_En(_json_data.GetParseError()),
                       err_offset,
                       config_file_contents.substr(std::max(0, err_offset - ERROR_DISPLAY_CHARS), ERROR_DISPLAY_CHARS)        );
        return JsonConfigReturnStatus::INVALID_FILE;
    }
    return JsonConfigReturnStatus::OK;
}

} // namespace jsonconfig
} // namespace sushi
