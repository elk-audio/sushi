/**
 * @Brief Wrapper for LV2 plugins - models.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2_features.h"

#include "logging.h"

namespace sushi {
namespace lv2 {

    MIND_GET_LOGGER_WITH_MODULE_NAME("lv2");

struct Port* port_by_symbol(LV2Model* model, const char* sym)
{
    for (uint32_t i = 0; i < model->num_ports; ++i)
    {
        struct Port* const port = &model->ports[i];
        const LilvNode* port_sym = lilv_port_get_symbol(model->plugin, port->lilv_port);

        if (!strcmp(lilv_node_as_string(port_sym), sym))
        {
            return port;
        }
    }

    return NULL;
}

int lv2_vprintf(LV2_Log_Handle handle,
                LV2_URID type,
                const char *fmt,
                va_list ap)
{
    LV2Model* model  = (LV2Model*)handle;
    if (type == model->urids.log_Trace && TRACE_OPTION)
    {
        MIND_LOG_WARNING("LV2 trace: {}", fmt);
    }
    else if (type == model->urids.log_Error)
    {
        MIND_LOG_ERROR("LV2 error: {}", fmt);
    }
    else if (type == model->urids.log_Warning)
    {
        MIND_LOG_WARNING("LV2 warning: {}", fmt);
    }

    return 0;
}

int lv2_printf(LV2_Log_Handle handle,
               LV2_URID type,
               const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int ret = lv2_vprintf(handle, type, fmt, args);
    va_end(args);
    return ret;
}

}
}

#endif //SUSHI_BUILD_WITH_LV2