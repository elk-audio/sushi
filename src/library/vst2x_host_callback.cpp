#include "twine/twine.h"

#include "vst2x_host_callback.h"
#include "vst2x_wrapper.h"
#include "logging.h"

namespace sushi {
namespace vst2 {

MIND_GET_LOGGER_WITH_MODULE_NAME("vst2");

VstIntPtr VSTCALLBACK host_callback(AEffect* effect, VstInt32 opcode, VstInt32 index,
                                    [[maybe_unused]] VstIntPtr value,
                                    [[maybe_unused]] void* ptr, float opt)
{
    VstIntPtr result = 0;

    MIND_LOG_DEBUG("PLUG> HostCallback (opcode {})\n index = {}, value = {}, ptr = {}, opt = {}\n", opcode, index, FromVstPtr<void> (value), ptr, opt);

    switch (opcode)
    {
        case audioMasterAutomate:
        {
            auto wrapper_instance = reinterpret_cast<Vst2xWrapper*>(effect->user);
            if (wrapper_instance == nullptr)
            {
                return 0; // Plugins could call this during initialisation, before the wrapper has finished construction
            }
            if (twine::is_current_thread_realtime())
            {
                auto wrapper_instance = reinterpret_cast<Vst2xWrapper*>(effect->user);
                if (twine::is_current_thread_realtime())
                {
                    wrapper_instance->notify_parameter_change_rt(index, opt);
                }
                else
                {
                    wrapper_instance->notify_parameter_change(index, opt);
                    MIND_LOG_DEBUG("Plugin {} sending parameter change notification: param: {}, value: {}",
                                   wrapper_instance->name(), index, opt);
                }
            }
            break;
        }

        case audioMasterVersion:
        {
            result = kVstVersion;
            break;
        }

        case audioMasterGetTime:
        {
            // Pass a pointer to a VstTimeInfo populated with updated data to the plugin
            auto wrapper_instance = reinterpret_cast<Vst2xWrapper*>(effect->user);
            if (wrapper_instance == nullptr)
            {
                return 0; // Plugins could call this during initialisation, before the wrapper has finished construction
            }
            result = reinterpret_cast<VstIntPtr>(wrapper_instance->time_info());
            break;
        }

        case audioMasterProcessEvents:
        {
            auto wrapper_instance = reinterpret_cast<Vst2xWrapper*>(effect->user);
            if (wrapper_instance == nullptr)
            {
                return 0; // Plugins could call this during initialisation, before the wrapper has finished construction
            }
            auto events = reinterpret_cast<VstEvents*>(ptr);
            for (int i = 0; i < events->numEvents; ++i)
            {
                wrapper_instance->output_vst_event(events->events[i]);
            }
            break;
        }

        default:
            break;

    }
    return result;

}

} // namespace vst2
} // namespace sushi
