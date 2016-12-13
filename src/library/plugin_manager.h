/**
 * @Brief Internal plugin manager.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */


#ifndef SUSHI_PLUGIN_MANAGER_H
#define SUSHI_PLUGIN_MANAGER_H

#include "plugin_interface.h"
#include "plugin_parameters.h"

#include <EASTL>

/**
 * @brief internal wrapper class for StompBox instances that keeps track
 * of all the host-related configuration.
 */

class StompBoxManager : public StompBoxController
{
public:

    FloatStompBoxParameter* register_float_parameter(std::string label,
                                                     std::string id,
                                                     float default_value = 0,
                                                     float max_value,
                                                     float min_value,
                                                     FloatParameterPreProcessor* customPreProcessor = nullptr) override;

    IntStompBoxParameter* register_int_parameter(std::string label,
                                                 std::string id,
                                                 int default_value,
                                                 int max_value,
                                                 int min_value,
                                                 IntParameterPreProcessor* customPreProcessor = nullptr) override;

    BoolStompBoxParameter* register_bool_parameter(std::string label,
                                                   std::string id,
                                                   bool default_value,
                                                   BoolParameterPreProcessor* customPreProcessor = nullptr) override;

    // 
    std::unique_ptr<StompBox> _instance;
private:
    void register_parameter(BaseStompBoxParameter* parameter);



    eastl::vector<std::unique_ptr<BaseStompBoxParameter>> _parameters;
};

#endif //SUSHI_PLUGIN_MANAGER_H
