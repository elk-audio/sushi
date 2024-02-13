/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Vst2 host callback implementation
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_VST2X_HOST_CALLBACK_H
#define SUSHI_VST2X_HOST_CALLBACK_H

#include "sushi/warning_suppressor.h"

ELK_PUSH_WARNING
ELK_DISABLE_UNUSED_PARAMETER
#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"
ELK_POP_WARNING

namespace sushi::internal::vst2 {

typedef AEffect *(*plugin_entry_proc)(audioMasterCallback host);

extern VstIntPtr VSTCALLBACK host_callback(AEffect* effect,
                                           VstInt32 opcode, VstInt32 index,
                                           VstIntPtr value, void* ptr, float opt);

} // end namespace sushi::internal::vst2

#endif // SUSHI_VST2X_HOST_CALLBACK_H
