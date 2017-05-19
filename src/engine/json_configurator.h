/**
 * @brief Class to configure the audio engine using Json config files.
 * @copyright MIND Music Labs AB, Stockholm
 */


#ifndef SUSHI_CONFIG_FROM_JSON_H
#define SUSHI_CONFIG_FROM_JSON_H

#include "engine/engine.h"
#include <json/json.h>

namespace sushi {
namespace jsonconfig {

enum class JsonConfigReturnStatus
{
    OK,
    INVALID_CHAIN_FORMAT,
    INVALID_CHAIN_MODE,
    INVALID_CHAIN_ID,
    INVALID_STOMPBOX_FORMAT,
    INVALID_STOMPBOX_UID,
    INVALID_STOMPBOX_NAME,
    INVALID_FILE
};

class JsonConfigurator
{
public:
    JsonConfigurator(engine::BaseEngine* engine) : _engine(engine) {}

    ~JsonConfigurator() {}

    /**
     * @brief reads a json config data and configures the engine with the specified chains.
     * @param path_to_file String which denotes the path of the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_chains(const std::string &path_to_file);

private:
    // pointer to the engine which will be initialized by this class.
    engine::BaseEngine* _engine;

    /**
     * @brief Helper function to parse the json file.
     * @param path_to_file String which denotes the path of the file.
     * @param config Json::Value object where the json data is stored after reading from the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _parse_file(const std::string& path_to_file, Json::Value& config);

    /**
     * @brief Creates a plugin chain with the specified number of channels and adds
     * the respective plugins to the chain if they are defined in the file.
     * @param chain_def Json::Value array representing a single plugin chain and its details.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _make_chain(const Json::Value & chain_def);

    /**
     * @brief checks of the Json data read from the file contains all the necessary definitions
     * to create a valid Plugin chain. Used by the function load_chains.
     * @param config Json::value containing the Json data read from the file.
     * @return JsonConfigReturnStatus::OK if definition is adept, different error code otherwise.
     */
    JsonConfigReturnStatus _validate_chains_definition(const Json::Value& config);
};

}/* namespace JSONCONFIG */
}/* namespace SUSHI */

#endif //SUSHI_CONFIG_FROM_JSON_H



