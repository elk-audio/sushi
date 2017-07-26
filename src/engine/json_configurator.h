/**
 * @brief Class to configure the audio engine using Json config files.
 * @copyright MIND Music Labs AB, Stockholm
 *
 * @details This file contains class JsonConfigurator which provides public methods
 * to read configuration data from Json config files, validate its schema and configure
 * the engine with it. It can load plugin chains and midi connections from the config file.
 */

#ifndef SUSHI_CONFIG_FROM_JSON_H
#define SUSHI_CONFIG_FROM_JSON_H

#include <json/json.h>

#include "engine/engine.h"
#include "engine/midi_dispatcher.h"

namespace sushi {
namespace jsonconfig {

enum class JsonConfigReturnStatus
{
    OK,
    INVALID_CHAIN_FORMAT,
    INVALID_CHAIN_MODE,
    INVALID_CHAIN_NAME,
    INVALID_PLUGIN_FORMAT,
    INVALID_PLUGIN_PATH,
    INVALID_PARAMETER,
    INVALID_PLUGIN_NAME,
    INVALID_PLUGIN_TYPE,
    INVALID_PLUGIN_UID,
    NO_MIDI_CONNECTIONS,
    INVALID_MIDI_CHAIN_CON,
    INVALID_MIDI_PORT,
    INVALID_MIDI_CHANNEL,
    INVALID_MIDI_CC_MAP,
    INVALID_CC_NUMBER,
    INVALID_MIDI_RANGE,
    INVALID_FILE
};

class JsonConfigurator
{
public:
    JsonConfigurator(engine::BaseEngine* engine,
                     midi_dispatcher::MidiDispatcher* midi_dispatcher) : _engine(engine),
                                                                         _midi_dispatcher(midi_dispatcher) {}

    ~JsonConfigurator() {}

    /**
     * @brief reads a json config file, searches for valid plugin chain
     * definitions and configures the engine with the specified chains.
     * @param path_to_file String which denotes the path of the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_chains(const std::string &path_to_file);

    /**
     * @brief reads a json config file, searches for valid MIDI connections and
     *        MIDI CC Mapping definitions and configures the engine with the specified MIDI information.
     * @param path_to_file String which denotes the path of the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_midi(const std::string &path_to_file);

private:
    /**
     * @brief Helper function to parse the json file using the JSONCPP library.
     *        Used by load_chains and load_midi.
     * @param path_to_file String which denotes the path of the file.
     * @param config Json::Value object where the json data is stored after reading from the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _parse_file(const std::string& path_to_file, Json::Value& config);

    /**
     * @brief Uses Engine's API to create a single plugin chain with the specified number of channels and adds
     *        the respective plugins to the chain if they are defined in the file. Used by load_chains.
     * @param chain_def Json::Value object representing a single plugin chain and its details.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _make_chain(const Json::Value & chain_def);

    /**
     * @brief Helper function to extract the number of midi channels in the midi definition.
     * @param channels Json::Value object containing the channel information parsed from the file.
     * @return The number of MIDI channels.
     */
    int _get_midi_channel(const Json::Value &channels);

    /**
     * @brief checks if the Json data read from the file contains all the necessary definitions
     *        to create a valid Plugin chain. Used by the function load_chains.
     * @param config Json::Value object containing the Json data read from the file.
     * @return JsonConfigReturnStatus::OK if definition is adept, different error code otherwise.
     */
    JsonConfigReturnStatus _validate_chains_definition(const Json::Value& config);

    /**
     * @brief Validates if the Json data read from the file has a valid Json schema
     *        for midi connections and midi mappings. Used by the function load_midi.
     * @param config Json::Value object containing the Json data read from the file.
     * @return JsonConfigReturnStatus::OK if definition is adept, different error code otherwise.
     */
    JsonConfigReturnStatus _validate_midi_definition(const Json::Value& config);

    /**
     * @brief Helper function used by _validate_midi_definition to validate the midi chain
     *        connections schema in the Json config file.
     * @param midi_def Json::Value object containing the MIDI Json data read from the file.
     * @return JsonConfigReturnStatus::OK if midi chain connections schema is valid,
     *         different error code otherwise.
     */
    JsonConfigReturnStatus _validate_midi_chain_connection_def(const Json::Value &midi_def);

    /**
     * @brief Helper function used by _validate_midi_definition to validate midi cc mapping schema
     *        in the Json config file.
     * @param midi_def Json::Value object containing the MIDI Json data read from the file.
     * @return JsonConfigReturnStatus::OK if midi chain connections schema is valid,
     *         different error code otherwise.
     */
    JsonConfigReturnStatus _validate_midi_cc_map_def(const Json::Value &midi_def);

    engine::BaseEngine* _engine;
    midi_dispatcher::MidiDispatcher* _midi_dispatcher;
};

}/* namespace JSONCONFIG */
}/* namespace SUSHI */

#endif //SUSHI_CONFIG_FROM_JSON_H



