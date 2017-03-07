/**
 * @Brief Wrapper for VST 2.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_VST2X_PLUGIN_H
#define SUSHI_VST2X_PLUGIN_H

#include "library/plugin_parameters.h"
#include "library/processor.h"

#include <algorithm>
#include <map>


namespace sushi {

/**
 * @brief internal wrapper class for loading VST plugins and make them accesible as Processor to the Engine.
 */

class Vst2xWrapper : public Processor
{
public:
    MIND_DECLARE_NON_COPYABLE(Vst2xWrapper)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */

    // TODO: call the Vst plugin loader, do the necessary stuff, check other things for being compatible now
    //       (e.g. number of channels, etc.), register the proper callbacks
    //       and prepare the internal parameter data structures needed for event processing
    Vst2xWrapper(const std::string& vst_plugin_path)
    {
    };

    // TODO: remember to deallocate everything, including dynamic library
    virtual ~Vst2xWrapper() {};

    /* Inherited from Processor */
    void process_event(BaseEvent* event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    // TODO: vedi se riesci a riciclare questi, altrimenti amen
    /**
    * @brief Return the parameter with the given unique id
    */
    BaseStompBoxParameter* get_parameter(const std::string& id)
    {
        auto parameter = _parameters.find(id);
        if (parameter == _parameters.end())
        {
            return nullptr;
        }
        return parameter->second.get();
    }

    void register_parameter(BaseStompBoxParameter* parameter)
    {
        _parameters.insert(std::pair<std::string, std::unique_ptr<BaseStompBoxParameter>>(parameter->id(),
                                                                                          std::unique_ptr<BaseStompBoxParameter>(parameter)));
    }

    std::map<std::string, std::unique_ptr<BaseStompBoxParameter>> _parameters;
};

} // end namespace sushi

#endif //SUSHI_VST2X_PLUGIN_H
