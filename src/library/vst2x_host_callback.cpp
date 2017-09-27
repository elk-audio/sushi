#include "vst2x_host_callback.h"
#include "logging.h"

namespace sushi {
namespace vst2 {

MIND_GET_LOGGER;

// Disable unused variable warnings as the host callback just print debug info atm
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
VstIntPtr VSTCALLBACK host_callback(AEffect* /* effect */,
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
    default:
        break;
    }

    return result;

}
#pragma GCC diagnostic pop

} // namespace vst2
} // namespace sushi
