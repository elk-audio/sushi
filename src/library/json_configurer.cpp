#include "json_configurer.h"

#include <fstream>
#include "logging.h"


namespace sushi {
namespace jsonconfigurer {

MIND_GET_LOGGER;

JsonConfigReturnStatus JsonConfigurer::init_configurer(engine::BaseEngine* engine, const std::string& path_to_file)
{
    if(path_to_file.empty())
    {
        MIND_LOG_ERROR("Empty file name passed to JsonConfigurer");
        return JsonConfigReturnStatus::INVALID_FILE;   
    }
    _engine = engine;
    std::ifstream file(path_to_file);
    Json::Reader reader;
    bool parse_ok = reader.parse(file, _config, false);
    if (!parse_ok)
    {
        MIND_LOG_ERROR("Error parsing JSON config file ({})", path_to_file);
        return JsonConfigReturnStatus::INVALID_FILE;
    }

    /*===== Move to respective methods which handle parsing of events and midi  ======*/
    
    // if(!_config.isMember("events"))
    // {
    //     MIND_LOG_ERROR("No events definition in JSON config file ({})", path_to_file);
    //     return JsonConfigReturnStatus::INVALID_FILE;
    // }
    // if(!_config.isMember("midi"))
    // {
    //     MIND_LOG_ERROR("No midi connection definition in JSON config file ({})", path_to_file);
    //     return JsonConfigReturnStatus::INVALID_FILE;
    // }
    MIND_LOG_INFO("Succesfully parsed JSON config file ({})", path_to_file);
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurer::init_chains_from_jsonconfig()
{
    JsonConfigReturnStatus status = _check_stompbox_chains_definition();
    if(status != JsonConfigReturnStatus::OK)
    {
        MIND_LOG_ERROR("Failed to initialize chains from JSON config file");
        return status;
    }  

    for (auto& chain : _config["stompbox_chains"])
    {
        status = _fill_chain(chain);
        if (status != JsonConfigReturnStatus::OK)
        {
            return status;
        }
    }
    MIND_LOG_INFO("Succesfully initialized chains from JSON config file");
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurer::_fill_chain(const Json::Value& chain_def)
{
    JsonConfigReturnStatus status_chaindef = _check_chain_definition(chain_def);
    if(status_chaindef != JsonConfigReturnStatus::OK)
    {
        return status_chaindef;
    }

    int num_channels = 0;
    if (chain_def["mode"] == "mono")
    {
        num_channels = 1;
    }
    else 
    {
        num_channels = 2;
    }

    auto chain_name = chain_def["id"].asString();
    engine::EngineReturnStatus status;
    status = _engine->create_empty_plugin_chain(chain_name, num_channels);
    if(status != engine::EngineReturnStatus::OK)
    {
        return JsonConfigReturnStatus::INVALID_CHAIN;
    }
    const Json::Value &stompbox_defs = chain_def["stompboxes"];
    for(const Json::Value &def : stompbox_defs)
    {
        auto uid = def["stompbox_uid"].asString();
        auto name = def["id"].asString();
        status = _engine->add_plugin_to_chain(chain_name, uid, name);
        if(status == engine::EngineReturnStatus::INVALID_STOMPBOX_UID)
        {
            return JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT;
        }
        else if(status == engine::EngineReturnStatus::INVALID_STOMPBOX_CHAIN)
        {
            return JsonConfigReturnStatus::INVALID_CHAIN;
        }
    }
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurer::_check_stompbox_chains_definition()
{
    /*======  Check if stompbox definitions exist in the file  ======*/
    if(!_config.isMember("stompbox_chains"))
    {
        MIND_LOG_ERROR("No stompbox chain definition in JSON config file");
        return JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT;
    }

    /*====== Check if stompbox definitions is an array of chain def  ======*/
    if (!_config["stompbox_chains"].isArray() || _config["stompbox_chains"].empty())
    {
        MIND_LOG_ERROR("Incorrect number of stompbox chains in configuration file");
        return JsonConfigReturnStatus::INVALID_CHAIN_SIZE;
    }
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurer::_check_chain_definition(const Json::Value& chain_def)
{
    /*====== Check if mode is a member  ======*/
    if(!chain_def.isMember("mode") || chain_def["mode"].empty())
    {
        MIND_LOG_ERROR("No chain mode definition in JSON config file");
        return JsonConfigReturnStatus::INVALID_FILE;
    }

    /*======  Check if mode is mono or stereo  ======*/
    std::string mode = chain_def["mode"].asString();
    if(mode != "mono" && mode != "stereo")
    {
        MIND_LOG_ERROR("Unrecognized channel configuration mode \"{}\"", mode);
        return JsonConfigReturnStatus::INVALID_CHAIN_MODE;
    }

    /*====== Check if chain id is present  ======*/
    if(!chain_def.isMember("id") || chain_def["id"].empty())
    {
        MIND_LOG_ERROR("Chain ID is not specified in configuration file");
        return JsonConfigReturnStatus::INVALID_CHAIN;
    }

    if(!chain_def.isMember("stompboxes") || chain_def["stompboxes"].empty() || !chain_def["stompboxes"].isArray())
    {
        MIND_LOG_ERROR("Invalid stompboxes definition in chain \"{}\"", chain_def["id"].asString());
        return JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT; 
    }

    /*====== Check for each stompbox definition UID and ID is present  ======*/
    for(const Json::Value &def : chain_def["stompboxes"])
    {
        bool uid_check = !def.isMember("stompbox_uid") || def["stompbox_uid"].empty();
        bool id_check = !def.isMember("id") || def["id"].empty();
        if(uid_check || id_check)
        {
            MIND_LOG_ERROR("Invalid stompboxes definition in chain \"{}\"", chain_def["id"].asString());
            return JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT;        
        }
    }
    return JsonConfigReturnStatus::OK;
}

} // namespace jsonconfigurer
} // namespace sushi
