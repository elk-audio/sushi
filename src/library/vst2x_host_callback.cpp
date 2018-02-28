#include "vst2x_host_callback.h"
#include "vst2x_wrapper.h"

#include "logging.h"

namespace sushi {
namespace vst2 {

MIND_GET_LOGGER;

// Disable unused variable warnings as the host callback just print debug info atm
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
VstIntPtr VSTCALLBACK host_callback(AEffect* effect,
                                    VstInt32 opcode, VstInt32 index,
                                    VstIntPtr value, void* ptr, float opt)
{
    VstIntPtr result = 0;

    MIND_LOG_DEBUG("PLUG> HostCallback (opcode {})\n index = {}, value = {}, ptr = {}, opt = {}\n", opcode, index, FromVstPtr<void> (value), ptr, opt);

    switch (opcode)
    {
    case audioMasterVersion :
        result = kVstVersion;
        break;

    case  audioMasterGetTime:
    {
        // Pass a pointer to a VstTimeInfo populated with updated data to the plugin
        auto wrapper_instance = reinterpret_cast<Vst2xWrapper*>(effect->user);
        result = reinterpret_cast<VstIntPtr>(wrapper_instance->time_info());
    }
    default:
        break;
    }

    return result;

}
#pragma GCC diagnostic pop

} // namespace vst2
} // namespace sushi
