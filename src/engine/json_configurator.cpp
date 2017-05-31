#include "json_configurator.h"

#include <fstream>
#include "logging.h"

namespace sushi {
namespace jsonconfig {

using namespace engine;
using namespace midi_dispatcher;

MIND_GET_LOGGER;

JsonConfigReturnStatus JsonConfigurator::load_chains(const std::string &path_to_file)
{
    Json::Value config;
    auto status = _parse_file(path_to_file, config);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    status = _validate_chains_definition(config);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    for (auto& chain : config["stompbox_chains"])
    {
        status = _make_chain(chain);
        if (status != JsonConfigReturnStatus::OK)
        {
            return status;
        }
    }
    MIND_LOG_INFO("Successfully configured engine with plugins chains in JSON config file {}", path_to_file);
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_midi(const std::string &path_to_file)
{
    Json::Value config;
    auto status = _parse_file(path_to_file, config);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    status = _validate_midi_definition(config);
    if(status != JsonConfigReturnStatus::OK)
    {
        return status;
    }
    const Json::Value& midi = config["midi"];
    for (const auto& con : midi["chain_connections"])
    {
        auto res = _midi_dispatcher->connect_kb_to_track(con["port"].asInt(),
                                                         con["chain"].asString(),
                                                         _get_midi_channel(con["channel"]));
        if (res != MidiDispatcherStatus::OK)
        {
            if(res == MidiDispatcherStatus::INVALID_MIDI_INPUT)
            {
                MIND_LOG_ERROR("Invalid \"port\" (type:int) specified for midi "
                                       "channel connections in Json Config file.");
                return JsonConfigReturnStatus::INVALID_MIDI_PORT;
            }
            MIND_LOG_ERROR("Invalid \"chain\" (type:string) for midi "
                                   "chain connection in Json config file.");
            return JsonConfigReturnStatus::INVALID_CHAIN_NAME;
        }
    }

    for (const auto& cc_map : midi["cc_mappings"])
    {
        auto res = _midi_dispatcher->connect_cc_to_parameter(cc_map["port"].asInt(),
                                                             cc_map["processor"].asString(),
                                                             cc_map["parameter"].asString(),
                                                             cc_map["cc_number"].asInt(),
                                                             cc_map["min_range"].asFloat(),
                                                             cc_map["max_range"].asFloat(),
                                                             _get_midi_channel(cc_map["channel"]));
        if (res != MidiDispatcherStatus::OK)
        {
            if(res == MidiDispatcherStatus::INVALID_MIDI_INPUT)
            {
                MIND_LOG_ERROR("Invalid \"port\" (type:int) specified "
                                       "for midi cc mappings in Json Config file.");
                return JsonConfigReturnStatus::INVALID_MIDI_PORT;
            }
            if(res == MidiDispatcherStatus::INVALID_PROCESSOR)
            {
                MIND_LOG_ERROR("Invalid processor: \"{}\" specified for midi cc mappings.",
                               cc_map["processor"].asString());
                return JsonConfigReturnStatus::INVALID_CHAIN_NAME;
            }
            MIND_LOG_ERROR("Invalid parameter \"{}\" specified for processor: \"{}\" for midi cc mappings.",
                           cc_map["parameter"].asString(), cc_map["processor"].asString());
            return JsonConfigReturnStatus::INVALID_PARAMETER;
        }
    }
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::_parse_file(const std::string& path_to_file, Json::Value& config)
{
    std::ifstream file(path_to_file);
    if(!file.good())
    {
        MIND_LOG_ERROR("Invalid file passed to JsonConfigurator {}", path_to_file);
        return JsonConfigReturnStatus::INVALID_FILE;
    }
    Json::Reader reader;
    bool parse_ok = reader.parse(file, config, false);
    if (!parse_ok)
    {
        MIND_LOG_ERROR("Error parsing JSON config file {}",  reader.getFormattedErrorMessages());
        return JsonConfigReturnStatus::INVALID_FILE;
    }
    MIND_LOG_INFO("Succesfully parsed JSON config file {}", path_to_file);
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::_make_chain(const Json::Value &chain_def)
{
    int num_channels = 0;
    if (chain_def["mode"] == "mono")
    {
        num_channels = 1;
    }
    else
    {
        num_channels = 2;
    }
    auto chain_name = chain_def["name"].asString();
    auto status = _engine->create_plugin_chain(chain_name, num_channels);
    if(status != EngineReturnStatus::OK)
    {
        MIND_LOG_ERROR("Plugin Chain Name {} in JSON config file already exists in engine", chain_name);
        return JsonConfigReturnStatus::INVALID_CHAIN_NAME;
    }

    for(const Json::Value &def : chain_def["stompboxes"])
    {
        auto stompbox_uid = def["stompbox_uid"].asString();
        auto stompbox_name = def["stompbox_name"].asString();
        status = _engine->add_plugin_to_chain(chain_name, stompbox_uid, stompbox_name);
        if(status != EngineReturnStatus::OK)
        {
            if(status == EngineReturnStatus::INVALID_STOMPBOX_UID)
            {
                MIND_LOG_ERROR("Incorrect stompbox UID {} in JSON config file", stompbox_uid);
                return JsonConfigReturnStatus::INVALID_STOMPBOX_UID;
            }
            MIND_LOG_ERROR("Stompbox Name {} in JSON config file already exists in engine", stompbox_name);
            return JsonConfigReturnStatus::INVALID_STOMPBOX_NAME;
        }
    }

    MIND_LOG_DEBUG("Successfully added Plugin Chain {} to the engine", chain_def["name"].asString());
    return JsonConfigReturnStatus::OK;
}

int JsonConfigurator::_get_midi_channel(const Json::Value &channels)
{
    if (channels.isString())
    {
        return midi::MidiChannel::OMNI;
    }
    return channels.asInt();
}

/* TODO: This is a basic validation, compare this to a schema using VALIJSON or similar library */
JsonConfigReturnStatus JsonConfigurator::_validate_chains_definition(const Json::Value& config)
{
    if(!config.isMember("stompbox_chains"))
    {
        MIND_LOG_ERROR("No plugin chains definitions in JSON config file");
        return JsonConfigReturnStatus::INVALID_CHAIN_FORMAT;
    }
    if (!config["stompbox_chains"].isArray() || config["stompbox_chains"].empty())
    {
        MIND_LOG_ERROR("Incorrect definition of plugin chains in JSON config file");
        return JsonConfigReturnStatus::INVALID_CHAIN_FORMAT;
    }

    /* Validate JSON scheme for each plugin chain defined */
    for (const auto& chain : config["stompbox_chains"])
    {
        if(!chain["name"].isString())
        {
            MIND_LOG_ERROR("\"name\" (type:string) for plugin "
                                   "chain is not defined in JSON config file");
            return JsonConfigReturnStatus::INVALID_CHAIN_NAME;
        }
        if(!chain["mode"].isString())
        {
            MIND_LOG_ERROR("\"mode\" (type:string) for plugin chain {} "
                                   "is not defined in JSON config file", chain["name"].asString());
            return JsonConfigReturnStatus::INVALID_CHAIN_MODE;
        }
        auto mode = chain["mode"].asString();
        if(mode != "mono" && mode != "stereo")
        {
            MIND_LOG_ERROR(" Unrecognized \"mode\" {} for plugin chain "
                                   "in Json config file.", mode);
            return JsonConfigReturnStatus::INVALID_CHAIN_MODE;
        }
        if(!chain.isMember("stompboxes") || !chain["stompboxes"].isArray())
        {
            MIND_LOG_ERROR("\"stompboxes\" is not defined for plugin chain "
                                   "\"{}\" in JSON config file", chain["name"].asString());
            return JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT;
        }
        if(!chain["stompboxes"].empty())
        {
            for(auto& def : chain["stompboxes"])
            {
                if(!def["stompbox_uid"].isString())
                {
                    MIND_LOG_ERROR("\"stompbox_uid\" (type:string) is not defined for plugin chain "
                                           "\"{}\" in JSON config file", chain["name"].asString());
                    return  JsonConfigReturnStatus::INVALID_STOMPBOX_UID;
                }
                if(!def["stompbox_name"].isString())
                {
                    MIND_LOG_ERROR("\"stompbox_name\" (type:string) is not defined for plugin chain "
                                           "\"{}\" in JSON config file", chain["name"].asString());
                    return  JsonConfigReturnStatus::INVALID_STOMPBOX_NAME;
                }
            }
        }
        else
        {
            MIND_LOG_DEBUG("Plugin chain {} has empty stompbox chain.", chain["name"].asString());
        }
    }
    MIND_LOG_DEBUG("Plugin chains definition in Json Config file follow valid schema.");
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::_validate_midi_definition(const Json::Value &config)
{
    if(config.empty() || !config.isMember("midi") || config["midi"].empty())
    {
        MIND_LOG_WARNING("No midi connection information in Json config file.");
        return JsonConfigReturnStatus::NO_MIDI_CONNECTIONS;
    }
    const Json::Value& midi_def = config["midi"];

    auto status = _validate_midi_chain_connection_def(midi_def);
    if(status != JsonConfigReturnStatus::OK)
    {
        MIND_LOG_ERROR("Error in definition of Midi chain connections in Json config file.");
        return status;
    }

    status = _validate_midi_cc_map_def(midi_def);
    if(status != JsonConfigReturnStatus::OK)
    {
        MIND_LOG_ERROR("Error in definition of Midi CC mappings in Json config file.");
        return status;
    }

    MIND_LOG_DEBUG("Midi follows definition in Json Config file follows a valid schema");
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::_validate_midi_chain_connection_def(const Json::Value &midi_def)
{
    if(!midi_def.isMember("chain_connections"))
    {
        MIND_LOG_ERROR("Midi chain connections are not defined in Json config file");
        return JsonConfigReturnStatus::INVALID_MIDI_CHAIN_CON;
    }
    if(!midi_def["chain_connections"].isArray() || midi_def["chain_connections"].empty())
    {
        MIND_LOG_ERROR("Midi chain connections are improperly defined in Json config file");
        return JsonConfigReturnStatus::INVALID_MIDI_CHAIN_CON;
    }
    for(const auto& con : midi_def["chain_connections"])
    {
        if(!con["port"].isInt())
        {
            MIND_LOG_ERROR("\"port\" (type:int) for midi "
                                   "chain connection is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_MIDI_PORT;
        }
        if(!con["chain"].isString())
        {
            MIND_LOG_ERROR("\"chain\" (type:string) for midi "
                                   "chain connection is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_CHAIN_NAME;
        }
        if(!con["channel"].isInt() && !con["channel"].isString())
        {
            MIND_LOG_ERROR("\"channel\" (type:string or int) for midi "
                                   "chain connection is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_MIDI_CHANNEL;
        }
        if(con["channel"].isString())
        {
            if(con["channel"].asString() != "omni" && con["channel"].asString() != "all")
            {
                MIND_LOG_ERROR(" Unrecognized \"channel\" {} for midi chain connection "
                                       "in Json config file.", con["channel"].asString());
                return JsonConfigReturnStatus::INVALID_MIDI_CHANNEL;
            }
        }
    }
    MIND_LOG_DEBUG("Midi connections definition in Json Config file follows a valid schema");
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::_validate_midi_cc_map_def(const Json::Value &midi_def)
{
    if(!midi_def.isMember("cc_mappings"))
    {
        MIND_LOG_ERROR("CC mappings are not defined in Json config file");
        return JsonConfigReturnStatus::INVALID_MIDI_CC_MAP;
    }
    if(!midi_def["cc_mappings"].isArray() || midi_def["cc_mappings"].empty())
    {
        MIND_LOG_ERROR("CC mappings are incorrectly defined in Json config file.");
        return JsonConfigReturnStatus::INVALID_MIDI_CC_MAP;
    }
    for(const auto& cc_map : midi_def["cc_mappings"])
    {
        if(!cc_map["port"].isInt())
        {
            MIND_LOG_ERROR("\"port\" (type:int) for midi "
                                   "cc mapping is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_MIDI_PORT;
        }
        if(!cc_map["channel"].isInt() && !cc_map["channel"].isString())
        {
            MIND_LOG_ERROR("\"channel\" (type:string or int) for midi "
                                   "cc mapping is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_MIDI_CHANNEL;
        }
        if(cc_map["channel"].isString())
        {
            if(cc_map["channel"].asString() != "omni" && cc_map["channel"].asString() != "all")
            {
                MIND_LOG_ERROR(" Unrecognized \"channel\" {} for midi chain connection "
                                       "in Json config file.", cc_map["channel"].asString());
                return JsonConfigReturnStatus::INVALID_MIDI_CHANNEL;
            }
        }
        if(!cc_map["cc_number"].isInt())
        {
            MIND_LOG_ERROR("\"cc_number\" (type:int) for midi "
                                   "cc mapping is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_CC_NUMBER;
        }
        if(!cc_map["processor"].isString())
        {
            MIND_LOG_ERROR("\"processor\" (type:int) for midi "
                                   "cc mapping is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_STOMPBOX_UID;
        }
        if(!cc_map["parameter"].isString())
        {
            MIND_LOG_ERROR("\"parameter\" (type:string) for midi "
                                   "cc mapping is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_PARAMETER;
        }
        if(!cc_map["min_range"].isInt())
        {
            MIND_LOG_ERROR("\"min_range\" (type:int) for "
                                   "midi cc mapping is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_MIDI_RANGE;
        }
        if(!cc_map["max_range"].isInt())
        {
            MIND_LOG_ERROR("\"max_range\" (type:int) for "
                                   "midi cc mapping is not defined in Json config file.");
            return JsonConfigReturnStatus::INVALID_MIDI_RANGE;
        }
    }
    MIND_LOG_DEBUG("Midi CC mappings definition in Json Config file follows a valid schema");
    return JsonConfigReturnStatus::OK;
}

} // namespace jsonconfig
} // namespace sushi
