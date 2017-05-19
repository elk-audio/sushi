#include "json_configurator.h"

#include <fstream>
#include "logging.h"

namespace sushi {
namespace jsonconfig {

using namespace engine;

MIND_GET_LOGGER;

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
    auto chain_name = chain_def["id"].asString();
    engine::EngineReturnStatus status;
    status = _engine->create_plugin_chain(chain_name, num_channels);
    if(status != EngineReturnStatus::OK)
    {
        /* Other error code can be caused only by already existing chain ID. */
        MIND_LOG_ERROR("Plugin Chain Name {} in Json config file already exists in engine", chain_name);
        return JsonConfigReturnStatus::INVALID_CHAIN_ID;
    }

    /* If no stompboxes are defined, return without adding any plugins.
     * Otherwise continue*/
    if(chain_def["stompboxes"].empty())
    {
        MIND_LOG_INFO("Plugin Chain {} in Json Config file is empty and has no stompboxes", chain_name);
        return JsonConfigReturnStatus::OK;
    }
    for(const Json::Value &def : chain_def["stompboxes"])
    {
        auto stompbox_uid = def["stompbox_uid"].asString();
        auto stompbox_name = def["id"].asString();
        status = _engine->add_plugin_to_chain(chain_name, stompbox_uid, stompbox_name);
        if(status != EngineReturnStatus::OK)
        {
            /* Error code is not "OK" for only one of the two possibilities respectively:
             * 1. Plugin UID is not correct.
             * 2. Already existing plugin name. */
            if(status == EngineReturnStatus::INVALID_STOMPBOX_UID)
            {
                MIND_LOG_ERROR("Incorrect Stompbox UID {} in Json config file", stompbox_uid);
                return JsonConfigReturnStatus::INVALID_STOMPBOX_UID;
            }
            MIND_LOG_ERROR("Stompbox Name {} in Json config file already exists in engine", stompbox_name);
            return JsonConfigReturnStatus::INVALID_STOMPBOX_NAME;
        }
    }
    MIND_LOG_DEBUG("Successfully added Plugin Chain {} to the engine", chain_def["id"].asString());
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::_validate_chains_definition(const Json::Value& config)
{
    if(!config.isMember("stompbox_chains"))
    {
        MIND_LOG_ERROR("No plugin chains definitions in JSON config file");
        return JsonConfigReturnStatus::INVALID_CHAIN_FORMAT;
    }
    if (!config["stompbox_chains"].isArray() || config["stompbox_chains"].empty())
    {
        MIND_LOG_ERROR("Incorrect definition of stompbox chains in configuration file");
        return JsonConfigReturnStatus::INVALID_CHAIN_FORMAT;
    }

    /* Validate JSON scheme for each plugin chain defined */
    for (auto& chain : config["stompbox_chains"])
    {
        if(!chain.isMember("mode") || chain["mode"].empty())
        {
            MIND_LOG_ERROR("No chain mode definition in JSON config file");
            return JsonConfigReturnStatus::INVALID_CHAIN_MODE;
        }
        std::string mode = chain["mode"].asString();
        if(mode != "mono" && mode != "stereo")
        {
            MIND_LOG_ERROR("Unrecognized channel configuration mode \"{}\"", mode);
            return JsonConfigReturnStatus::INVALID_CHAIN_MODE;
        }
        if(!chain.isMember("id") || chain["id"].empty())
        {
            MIND_LOG_ERROR("Chain ID is not specified in configuration file");
            return JsonConfigReturnStatus::INVALID_CHAIN_ID;
        }
        if(!chain.isMember("stompboxes") || !chain["stompboxes"].isArray())
        {
            MIND_LOG_ERROR("Invalid stompbox definition in plugin chain \"{}\"", chain["id"].asString());
            return JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT;
        }
        /* Check stompbox definitions if they are defined */
        if(!chain["stompboxes"].empty())
        {
            for(const Json::Value &def : chain["stompboxes"])
            {
                if(!def.isMember("stompbox_uid") || def["stompbox_uid"].empty())
                {
                    MIND_LOG_ERROR("Invalid stompbox UID in plugin chain \"{}\"", chain["id"].asString());
                    return  JsonConfigReturnStatus::INVALID_STOMPBOX_UID;
                }
                if(!def.isMember("id") || def["id"].empty())
                {
                    MIND_LOG_ERROR("Invalid stompbox name in plugin chain \"{}\"", chain["id"].asString());
                    return  JsonConfigReturnStatus::INVALID_STOMPBOX_NAME;
                }
            }
        }
        else
        {
            MIND_LOG_DEBUG("Plugin chain {} has empty stompbox chain.", chain["id"].asString());
        }
    }
    MIND_LOG_DEBUG("Plugin chains in Json Config file follow valid schema.");
    return JsonConfigReturnStatus::OK;
}

JsonConfigReturnStatus JsonConfigurator::load_chains(const std::string &path_to_file)
{
    Json::Value config;
    JsonConfigReturnStatus status = _parse_file(path_to_file, config);
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

} // namespace jsonconfigurer
} // namespace sushi
