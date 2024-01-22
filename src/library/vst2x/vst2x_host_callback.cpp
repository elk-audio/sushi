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

#include "twine/twine.h"
#include "elklog/static_logger.h"

#include "vst2x_host_callback.h"

#include "vst2x_wrapper.h"

namespace sushi::internal::vst2 {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("vst2");

VstIntPtr VSTCALLBACK host_callback(AEffect* effect, VstInt32 opcode, VstInt32 index,
                                    [[maybe_unused]] VstIntPtr value,
                                    [[maybe_unused]] void* ptr, float opt)
{
    VstIntPtr result = 0;

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
                wrapper_instance->notify_parameter_change_rt(index, opt);
            }
            else
            {
                wrapper_instance->notify_parameter_change(index, opt);
                ELKLOG_LOG_DEBUG("Plugin {} sending parameter change notification: param: {}, value: {}",
                                wrapper_instance->name(), index, opt);
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

} // end namespace sushi::internal::vst2
