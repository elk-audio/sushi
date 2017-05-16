/**
 * @brief json parser to initialize engine from Json config giles
 * @copyright MIND Music Labs AB, Stockholm
 */


#ifndef SUSHI_CONFIG_FROM_JSON_H
#define SUSHI_CONFIG_FROM_JSON_H

#include "engine/engine.h"

#include <json/json.h>

namespace sushi {
namespace jsonconfigurer {


enum class JsonConfigReturnStatus
{
    OK,
    INVALID_CHAIN,
    INVALID_CHAIN_MODE,
    INVALID_CHAIN_SIZE,
    INVALID_STOMPBOX_FORMAT,
    INVALID_FILE
};

class JsonConfigurer
{
public:
    JsonConfigurer() : _engine(nullptr) {}
    ~JsonConfigurer() {}

    /**
     * @brief reads the config file and checks if parsing was ok.
     * @param engine::BaseEngine* Pointer to the engine which it should modifly.
     * @param path_to_file The full path of the file.
     * eg: \home\Downloads\config.json.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus init_configurer(engine::BaseEngine* /*_engine*/, const std::string& /*path_to_file*/);

    /**
     * @brief checks if the stompbox chains are defined properly in the json config. If so it inititializes 
     * and adds plugin chains to the engine. Calls function "_fill_chain" for each stompbox chain defined in 
     * the config file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus init_chains_from_jsonconfig();
private:
    // pointer to the engine which will be initialized by this class.
    engine::BaseEngine* _engine;
    // Json Value containing all the json information present in the file. 
    Json::Value _config;

    /**
     * @brief checks if each stompbox chain is defined properly in the json config. If so it creates a
     * plugin chain with the specified number of channels and adds the respective plugins to the chain.
     * @param json value array representing a single plugin chain and its components.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _fill_chain(const Json::Value& /*chain_def*/);

    /**
     * @brief Used by _fill_chain to check if the stompbox chain is properly defined and has all the required parameters. 
     * Used to ensure incorrectly defined json config files DO NOT modify the engine. 
     * @param json value array representing a single plugin chain and its components.
     * @return JsonConfigReturnStatus::OK if definition is adept, different error code otherwise.
     */
    JsonConfigReturnStatus _check_chain_definition(const Json::Value& /*chain_def*/);

    /**
     * @brief Used by init_chains_from_jsonconfig to ensure multiple stompbox chains definitions are present in the file. 
     * Used to ensure incorrectly defined json config files DO NOT modify the engine. 
     * @return JsonConfigReturnStatus::OK if definition is adept, different error code otherwise.
     */
    JsonConfigReturnStatus _check_stompbox_chains_definition();    
};


}/*----------  namespace jsonconfigurer ----------*/
}/*----------  namespace SUSHI ----------*/

#endif //SUSHI_CONFIG_FROM_JSON_H



