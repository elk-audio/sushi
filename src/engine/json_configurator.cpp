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

MIND_GET_LOGGER_WITH_MODULE_NAME("jsonconfig");

JsonConfigReturnStatus JsonConfigurator::load_host_config(const std::string& path_to_file)
{
    rapidjson::Document config;
    auto status = _parse_file(path_to_file, config, JsonSection::HOST_CONFIG);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    const auto& host_config = config["host_config"].GetObject();
    float sample_rate = host_config["samplerate"].GetFloat();
    MIND_LOG_INFO("Setting engine sample rate to {}", sample_rate);
    _engine->set_sample_rate(sample_rate);

    if (host_config.HasMember("tempo"))
    {
        float tempo = host_config["tempo"].GetFloat();
        MIND_LOG_INFO("Setting engine tempo to {}", sample_rate);
        _engine->set_tempo(tempo);
    }

    if (host_config.HasMember("time_signature"))
    {
        const auto& sig = host_config["time_signature"].GetObject();
        int numerator = sig["numerator"].GetInt();
        int denominator = sig["denominator"].GetInt();

        MIND_LOG_INFO("Setting engine time signature to {}/{}", numerator, denominator);
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
        MIND_LOG_INFO("Setting engine playing mode to {}", mode == PlayingMode::PLAYING? "playing " : "stopped");
        _engine->set_transport_mode(mode);
    }

    if (host_config.HasMember("tempo_sync"))
    {
        SyncMode mode;
        if (host_config["tempo_sync"] == "ableton link")
        {
            mode = SyncMode::ABLETON_LINK;
        }
        else if (host_config["tempo_sync"] == "midi")
        {
            mode = SyncMode::MIDI_SLAVE;
        }
        else
        {
            mode = SyncMode::INTERNAL;
        }
        MIND_LOG_INFO("Setting engine tempo sync mode to {}", mode == SyncMode::ABLETON_LINK? "Ableton Link" : (
                                                              mode == SyncMode::MIDI_SLAVE? "external Midi" : "internal"));
        _engine->set_tempo_sync_mode(mode);
    }

    if (host_config.HasMember("audio_clip_detection"))
    {
        const auto& clip_det = host_config["audio_clip_detection"].GetObject();
        if (clip_det.HasMember("inputs"))
        {
            _engine->enable_input_clip_detection(clip_det["inputs"].GetBool());
            MIND_LOG_INFO("Setting engine input clip detection {}", clip_det["inputs"].GetBool()? "enabled" : "disabled");
        }
        if (clip_det.HasMember("outputs"))
        {
            _engine->enable_output_clip_detection(clip_det["outputs"].GetBool());
            MIND_LOG_INFO("Setting engine output clip detection {}", clip_det["outputs"].GetBool()? "enabled" : "disabled");
        }
    }

    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_tracks(const std::string &path_to_file)
{
    rapidjson::Document config;
    auto status = _parse_file(path_to_file, config, JsonSection::TRACKS);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }

    for (auto& track : config["tracks"].GetArray())
    {
        status = _make_track(track);
        if (status != JsonConfigReturnStatus::OK)
        {
            return status;
        }
    }
    MIND_LOG_INFO("Successfully configured engine with tracks in JSON config file \"{}\"", path_to_file);
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_midi(const std::string& path_to_file)
{
    rapidjson::Document config;
    auto status = _parse_file(path_to_file, config, JsonSection::MIDI);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }

    const rapidjson::Value& midi = config["midi"];
    if(midi.HasMember("track_connections"))
    {
        for (const auto& con : midi["track_connections"].GetArray())
        {
            bool raw_midi = con["raw_midi"].GetBool();
            MidiDispatcherStatus res;
            if (raw_midi)
            {
                res = _midi_dispatcher->connect_raw_midi_to_track(con["port"].GetInt(),
                                                                  con["track"].GetString(),
                                                                  _get_midi_channel(con["channel"]));

            }
            else
            {
                res = _midi_dispatcher->connect_kb_to_track(con["port"].GetInt(),
                                                            con["track"].GetString(),
                                                            _get_midi_channel(con["channel"]));
            }
            if (res != MidiDispatcherStatus::OK)
            {
                if(res == MidiDispatcherStatus::INVALID_MIDI_INPUT)
                {
                    MIND_LOG_ERROR("Invalid port \"{}\" specified specified for midi "
                                           "channel connections in Json Config file.", con["port"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
                MIND_LOG_ERROR("Invalid plugin track \"{}\" for midi "
                                       "track connection in Json config file.", con["track"].GetString());
                return JsonConfigReturnStatus::INVALID_TRACK_NAME;
            }
        }
    }

    if(midi.HasMember("track_out_connections"))
    {
        for (const auto& con : midi["track_out_connections"].GetArray())
        {
            auto res = _midi_dispatcher->connect_track_to_output(con["port"].GetInt(),
                                                                 con["track"].GetString(),
                                                                 _get_midi_channel(con["channel"]));
            if (res != MidiDispatcherStatus::OK)
            {
                if(res == MidiDispatcherStatus::INVALID_MIDI_OUTPUT)
                {
                    MIND_LOG_ERROR("Invalid port \"{}\" specified specified for midi "
                                           "channel connections in Json Config file.", con["port"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
                MIND_LOG_ERROR("Invalid plugin track \"{}\" for midi "
                                       "track connection in Json config file.", con["track"].GetString());
                return JsonConfigReturnStatus::INVALID_TRACK_NAME;
            }
        }
    }

    if(midi.HasMember("program_change_connections"))
    {
        for (const auto& con : midi["program_change_connections"].GetArray())
        {
            auto res = _midi_dispatcher->connect_pc_to_processor(con["port"].GetInt(),
                                                                 con["plugin"].GetString(),
                                                                 _get_midi_channel(con["channel"]));
            if (res != MidiDispatcherStatus::OK)
            {
                if(res == MidiDispatcherStatus::INVALID_MIDI_INPUT)
                {
                    MIND_LOG_ERROR("Invalid port \"{}\" specified specified for MIDI program change "
                                   "channel connections in Json Config file.", con["port"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
                MIND_LOG_ERROR("Invalid plugin \"{}\" for MIDI program change "
                               "connection in Json config file.", con["track"].GetString());
                return JsonConfigReturnStatus::INVALID_TRACK_NAME;
            }
        }
    }

    if(config["midi"].HasMember("cc_mappings"))
    {
        for (const auto& cc_map : midi["cc_mappings"].GetArray())
        {
            bool is_relative = false;
            if (cc_map.HasMember("relative"))
            {
                is_relative = cc_map["relative"].GetBool();
            }

            auto res = _midi_dispatcher->connect_cc_to_parameter(cc_map["port"].GetInt(),
                                                                 cc_map["plugin_name"].GetString(),
                                                                 cc_map["parameter_name"].GetString(),
                                                                 cc_map["cc_number"].GetInt(),
                                                                 cc_map["min_range"].GetFloat(),
                                                                 cc_map["max_range"].GetFloat(),
                                                                 is_relative,
                                                                 _get_midi_channel(cc_map["channel"]));
            if (res != MidiDispatcherStatus::OK)
            {
                if(res == MidiDispatcherStatus::INVALID_MIDI_INPUT)
                {
                    MIND_LOG_ERROR("Invalid port \"{}\" specified "
                                           "for midi cc mappings in Json Config file.", cc_map["port"].GetInt());
                    return JsonConfigReturnStatus::INVALID_MIDI_PORT;
                }
                if(res == MidiDispatcherStatus::INVALID_PROCESSOR)
                {
                    MIND_LOG_ERROR("Invalid plugin name \"{}\" specified "
                                           "for midi cc mappings in Json Config file.", cc_map["plugin_name"].GetString());
                    return JsonConfigReturnStatus::INVALID_TRACK_NAME;
                }
                MIND_LOG_ERROR("Invalid parameter name \"{}\" specified for plugin \"{}\" for midi cc mappings.",
                                                                            cc_map["parameter_name"].GetString(),
                                                                            cc_map["plugin_name"].GetString());
                return JsonConfigReturnStatus::INVALID_PARAMETER;
            }
        }
    }

    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_events(const std::string& path_to_file)
{
    rapidjson::Document events;
    auto status = _parse_file(path_to_file, events, JsonSection::EVENTS);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    auto dispatcher = _engine->event_dispatcher();
    for (auto& json_event : events["events"].GetArray())
    {
        if (Event* e = _parse_event(json_event, IGNORE_TIMESTAMP); e != nullptr)
            dispatcher->post_event(e);
    }
    return JsonConfigReturnStatus::OK;
}

std::pair<JsonConfigReturnStatus, std::vector<Event*>>
JsonConfigurator::load_event_list(const std::string& path_to_file)
{
    rapidjson::Document json_events;
    std::vector<Event*> events;
    auto status = _parse_file(path_to_file, json_events, JsonSection::EVENTS);
    if(status != JsonConfigReturnStatus::OK)
    {
        return std::make_pair(status, events);
    }
    for (auto& json_event : json_events["events"].GetArray())
    {
        if (Event* e = _parse_event(json_event, USE_TIMESTAMP); e != nullptr)
            events.push_back(e);
    }
    return std::make_pair(JsonConfigReturnStatus::OK, events);
}


JsonConfigReturnStatus JsonConfigurator::_parse_file(const std::string& path_to_file,
                                                     rapidjson::Document& config,
                                                     JsonSection section)
{
    std::ifstream config_file(path_to_file);
    if(!config_file.good())
    {
        MIND_LOG_ERROR("Invalid file passed to JsonConfigurator {}", path_to_file);
        return JsonConfigReturnStatus::INVALID_FILE;
    }
    //iterate through every char in file and store in the string
    std::string config_file_contents((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
    config.Parse(config_file_contents.c_str());
    if(config.HasParseError())
    {
        MIND_LOG_ERROR("Error parsing JSON config file: {}",  rapidjson::GetParseError_En(config.GetParseError()));
        return JsonConfigReturnStatus::INVALID_FILE;
    }

    switch(section)
    {
        case JsonSection::MIDI:
            if(!config.HasMember("midi"))
            {
                MIND_LOG_DEBUG("Config file does not have MIDI definitions");
                return JsonConfigReturnStatus::NO_MIDI_DEFINITIONS;
            }
            break;

        case JsonSection::EVENTS:
            if(!config.HasMember("events"))
            {
                MIND_LOG_DEBUG("Config file does not have events definitions");
                return JsonConfigReturnStatus::NO_EVENTS_DEFINITIONS;
            }
            break;

        default:
            break;
    }

    if(!_validate_against_schema(config, section))
    {
        MIND_LOG_ERROR("Config file {} does not follow schema", path_to_file);
        return JsonConfigReturnStatus::INVALID_CONFIGURATION;
    }

    MIND_LOG_INFO("Successfully parsed JSON config file {}", path_to_file);
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::_make_track(const rapidjson::Value &track_def)
{
    auto name = track_def["name"].GetString();
    EngineReturnStatus status = EngineReturnStatus::ERROR;
    if (track_def["mode"] == "mono")
    {
        status = _engine->create_track(name, 1);
    }
    else if (track_def["mode"] == "stereo")
    {
        status = _engine->create_track(name, 2);
    }
    else if (track_def["mode"] == "multibus")
    {
        if (track_def.HasMember("input_busses") && track_def.HasMember("output_busses"))
        {
            status = _engine->create_multibus_track(name, track_def["input_busses"].GetInt(),
                                                    track_def["output_busses"].GetInt());
        }
    }

    if(status == EngineReturnStatus::INVALID_PLUGIN_NAME || status == EngineReturnStatus::INVALID_PROCESSOR)
    {
        MIND_LOG_ERROR("Track {} in JSON config file duplicate or invalid name", name);
        return JsonConfigReturnStatus::INVALID_TRACK_NAME;
    }
    if(status != EngineReturnStatus::OK)
    {
        MIND_LOG_ERROR("Track Name {} failed to create", name);
        return JsonConfigReturnStatus::INVALID_CONFIGURATION;
    }

    MIND_LOG_DEBUG("Successfully added track \"{}\" to the engine", name);

    for(const auto& con : track_def["inputs"].GetArray())
    {
        if (con.HasMember("engine_bus"))
        {
            status = _engine->connect_audio_input_bus(con["engine_bus"].GetInt(), con["track_bus"].GetInt(), name);
        }
        else
        {
            status = _engine->connect_audio_input_channel(con["engine_channel"].GetInt(), con["track_channel"].GetInt(), name);
        }
        if(status != EngineReturnStatus::OK)
        {
            MIND_LOG_ERROR("Error connection input bus to track \"{}\", error {}", name, static_cast<int>(status));
            return JsonConfigReturnStatus::INVALID_CONFIGURATION;
        }
    }

    for(const auto& con : track_def["outputs"].GetArray())
    {
        if (con.HasMember("engine_bus"))
        {
            status = _engine->connect_audio_output_bus(con["engine_bus"].GetInt(), con["track_bus"].GetInt(), name);
        }
        else
        {
            status = _engine->connect_audio_output_channel(con["engine_channel"].GetInt(), con["track_channel"].GetInt(), name);

        }
        if(status != EngineReturnStatus::OK)
        {
            MIND_LOG_ERROR("Error connection track \"{}\" to output bus, error {}", name, static_cast<int>(status));
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
        else
        {
            plugin_uid = def["uid"].GetString();
            plugin_path = def["path"].GetString();
            plugin_type = PluginType::VST3X;
        }

        status = _engine->add_plugin_to_track(name, plugin_uid, plugin_name, plugin_path, plugin_type);
        if(status != EngineReturnStatus::OK)
        {
            if(status == EngineReturnStatus::INVALID_PLUGIN_UID)
            {
                MIND_LOG_ERROR("Invalid plugin uid {} in JSON config file", plugin_uid);
                return JsonConfigReturnStatus::INVALID_PLUGIN_PATH;
            }
            MIND_LOG_ERROR("Plugin Name {} in JSON config file already exists in engine", plugin_name);
            return JsonConfigReturnStatus::INVALID_PLUGIN_NAME;
        }
        MIND_LOG_DEBUG("Successfully added Plugin \"{}\" to"
                               " Chain \"{}\"", plugin_name, name);
    }

    MIND_LOG_DEBUG("Successfully added Track {} to the engine", name);
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
    auto [status, processor_id] = _engine->processor_id_from_name(data["plugin_name"].GetString());
    if (status != sushi::engine::EngineReturnStatus::OK)
    {
        MIND_LOG_WARNING("Unrecognised plugin: \"{}\"", data["plugin_name"].GetString());
        return nullptr;
    }
    if (json_event["type"] == "parameter_change")
    {
        auto [status, parameter_id] = _engine->parameter_id_from_name(data["plugin_name"].GetString(),
                                                                      data["parameter_name"].GetString());
        if (status != sushi::engine::EngineReturnStatus::OK)
        {
            MIND_LOG_WARNING("Unrecognised parameter: {}", data["parameter_name"].GetString());
            return nullptr;
        }
        return new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                        processor_id,
                                        parameter_id,
                                        data["value"].GetFloat(),
                                        timestamp);
    }
    if (json_event["type"] == "property_change")
    {
        auto [status, parameter_id] = _engine->parameter_id_from_name(data["plugin_name"].GetString(),
                                                                      data["property_name"].GetString());
        if (status != sushi::engine::EngineReturnStatus::OK)
        {
            MIND_LOG_WARNING("Unrecognised property: {}", data["property_name"].GetString());
            return nullptr;
        }
        return new StringPropertyChangeEvent(processor_id,
                                             parameter_id,
                                             data["value"].GetString(),
                                             timestamp);
    }
    if (json_event["type"] == "note_on")
    {
        return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON,
                                 processor_id,
                                 0, // channel
                                 data["note"].GetUint(),
                                 data["velocity"].GetFloat(),
                                 timestamp);
    }
    if (json_event["type"] == "note_off")
    {
        return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF,
                                 processor_id,
                                 0, // channel
                                 data["note"].GetUint(),
                                 data["velocity"].GetFloat(),
                                 timestamp);
    }
    return nullptr;
}

bool JsonConfigurator::_validate_against_schema(rapidjson::Document& config, JsonSection section)
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
        if(error_node.empty())
        {
            MIND_LOG_ERROR("Invalid Json Config File: missing definitions in the root of the document");
        }
        else
        {
            MIND_LOG_ERROR("Invalid Json Config File: Incorrect definition at {}", error_node);
        }
        return false;
    }
    return true;
}

} // namespace jsonconfig
} // namespace sushi
