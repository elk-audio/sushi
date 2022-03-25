/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Vst2 host callback implementation
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_VST2X_HOST_CALLBACK_H
#define SUSHI_VST2X_HOST_CALLBACK_H

#pragma GCC diagnostic push
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
