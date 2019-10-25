#ifndef SUSHI_VST2X_HOST_CALLBACK_H
#define SUSHI_VST2X_HOST_CALLBACK_H

#pragma GCC diagnostic ignored "-Wunused-parameter"
#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"
#pragma GCC diagnostic pop

namespace sushi {
namespace vst2 {

typedef AEffect *(*plugin_entry_proc)(audioMasterCallback host);

extern VstIntPtr VSTCALLBACK host_callback(AEffect* effect,
                                           VstInt32 opcode, VstInt32 index,
                                           VstIntPtr value, void* ptr, float opt);

} // namespace vst2
} // namespace sushi

#endif //SUSHI_VST2X_HOST_CALLBACK_H
