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

#include "rapidjson/document.h"

#include "engine/engine.h"
#include "engine/midi_dispatcher.h"

namespace sushi {
namespace jsonconfig {

enum class JsonConfigReturnStatus
{
    OK,
    INVALID_SCHEMA,
    INVALID_CHAIN_NAME,
    INVALID_PLUGIN_PATH,
    INVALID_PARAMETER,
    INVALID_PLUGIN_NAME,
    INVALID_MIDI_PORT,
    INVALID_FILE
};

enum class JsonSection
{
    HOST_CONFIG,
    CHAINS,
    MIDI,
    EVENTS
};

class JsonConfigurator
{
public:
    JsonConfigurator(engine::BaseEngine* engine,
                     midi_dispatcher::MidiDispatcher* midi_dispatcher) : _engine(engine),
                                                                         _midi_dispatcher(midi_dispatcher) {}

    ~JsonConfigurator() {}

    /**
     * @brief reads a json config file and set the given host configuration options
     * @param path_to_file String which denotes the path of the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_host_config(const std::string& path_to_file);

    /**
     * @brief reads a json config file, searches for valid plugin chain
     * definitions and configures the engine with the specified chains.
     * @param path_to_file String which denotes the path of the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_chains(const std::string& path_to_file);

    /**
     * @brief reads a json config file, searches for valid MIDI connections and
     *        MIDI CC Mapping definitions and configures the engine with the specified MIDI information.
     * @param path_to_file String which denotes the path of the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_midi(const std::string& path_to_file);

    /**
     * @brief reads a json config file, searches for a valid "events" definition and parses it
     *        into the rapidjson document.
     * @param path_to_file String which denotes the path of the file.
     * @param config rapidjson document which contains the events definitions
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus parse_events_from_file(const std::string& path_to_file, rapidjson::Document& config);

private:
    /**
     * @brief Helper function to parse the json file using the JSONCPP library.
     *        Used by load_chains and load_midi.
     * @param path_to_file String which denotes the path of the file.
     * @param config rapidjson document object where the json data is stored after reading from the file.
     * @param section Jsonsection to denote which section is to be validated.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _parse_file(const std::string& path_to_file, rapidjson::Document& config, JsonSection section);

    /**
     * @brief Uses Engine's API to create a single plugin chain with the specified number of channels and adds
     *        the respective plugins to the chain if they are defined in the file. Used by load_chains.
     * @param chain_def rapidjson document object representing a single plugin chain and its details.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _make_chain(const rapidjson::Value& chain_def);

    /**
     * @brief Helper function to extract the number of midi channels in the midi definition.
     * @param channels rapidjson document object containing the channel information parsed from the file.
     * @return The number of MIDI channels.
     */
    int _get_midi_channel(const rapidjson::Value& channels);

    /**
     * @brief function which validates the json data against the respective schema.
     * @param config rapidjson document object containing the json data parsed from the file
     * @param section JsonSection to denote which json section is to be validated.
     * @return true if json follows schema, false otherwise
     */
    bool _validate_against_schema(rapidjson::Document& config, JsonSection section);

    engine::BaseEngine* _engine;
    midi_dispatcher::MidiDispatcher* _midi_dispatcher;
};

}/* namespace JSONCONFIG */
}/* namespace SUSHI */

#endif //SUSHI_CONFIG_FROM_JSON_H