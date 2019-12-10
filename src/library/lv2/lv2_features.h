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
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - extra features.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_LV2_FEATURES_H
#define SUSHI_LV2_FEATURES_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <map>
#include <mutex>

// Temporary - just to check that it finds them.
#include <lilv-0/lilv/lilv.h>

#include "lv2/resize-port/resize-port.h"
#include "lv2/midi/midi.h"
#include "lv2/log/log.h"
#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/buf-size/buf-size.h"
#include "lv2/data-access/data-access.h"
#include "lv2/options/options.h"
#include "lv2/parameters/parameters.h"
#include "lv2/patch/patch.h"
#include "lv2/port-groups/port-groups.h"
#include "lv2/port-props/port-props.h"
#include "lv2/presets/presets.h"
#include "lv2/state/state.h"
#include "lv2/time/time.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"
#include "lv2/worker/worker.h"

#include "../processor.h"
#include "../../engine/base_event_dispatcher.h"

#include "lv2_symap.h"
#include "lv2_evbuf.h"
#include "lv2_model.h"

namespace sushi {
namespace lv2 {
// writing also LV2 Trace Log messages to file.
static const bool TRACE_OPTION = true;

static inline bool lv2_ansi_start(FILE *stream, int color)
{
// TODO: What is this.
#ifdef HAVE_ISATTY
if (isatty(fileno(stream))) {
return fprintf(stream, "\033[0;%dm", color);
}
#endif
return 0;
}

/**
   Get a port structure by symbol.

   (Inherited) TODO: Build an index to make this faster, currently O(n) which may be
   a problem when restoring the state of plugins with many ports.
*/
Port* port_by_symbol(LV2Model* model, const char* sym);

int lv2_vprintf(LV2_Log_Handle handle,
            LV2_URID type,
            const char *fmt,
            va_list ap);

int lv2_printf(LV2_Log_Handle handle,
           LV2_URID type,
           const char *fmt, ...);

typedef int (*PresetSink)(LV2Model* model,
                          const LilvNode* node,
                          const LilvNode* title,
                          void* data);

char* make_path(LV2_State_Make_Path_Handle handle, const char* path);

LV2_URID map_uri(LV2_URID_Map_Handle handle, const char* uri);

const char* unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid);

void init_feature(LV2_Feature* const dest, const char* const URI, void* data);

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

// (...)

#endif

#endif //SUSHI_LV2_FEATURES_H