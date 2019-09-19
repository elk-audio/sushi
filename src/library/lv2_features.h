/**
 * @Brief Wrapper for LV2 plugins - extra features.
 * @copyright MIND Music Labs AB, Stockholm
 *
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

#include "processor.h"

#include "lv2_symap.h"

#include "../engine/base_event_dispatcher.h"
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
struct Port* port_by_symbol(LV2Model* model, const char* sym);

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

static char* lv2_strjoin(const char* a, const char* b)
{
    const size_t a_len = strlen(a);
    const size_t b_len = strlen(b);
    char* const out = (char*)malloc(a_len + b_len + 1);

    memcpy(out, a, a_len);
    memcpy(out + a_len, b, b_len);
    out[a_len + b_len] = '\0';

    return out;
}

// Ilias TODO: Currently allocated plugin instances are not automatically freed when the _loader is destroyed. Should they be?

static LV2_URID map_uri(LV2_URID_Map_Handle handle, const char* uri)
{
    LV2Model* model = (LV2Model*)handle;
    std::unique_lock<std::mutex> lock(model->symap_lock); // Added by Ilias, replacing ZixSem
    const LV2_URID id = symap_map(model->symap, uri);

    return id;
}

static const char* unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid)
{
    LV2Model* model = (LV2Model*)handle;
    std::unique_lock<std::mutex> lock(model->symap_lock);  // Added by Ilias, replacing ZixSem
    const char* uri = symap_unmap(model->symap, urid);
    return uri;
}

static void init_feature(LV2_Feature* const dest, const char* const URI, void* data)
{
    dest->URI = URI;
    dest->data = data;
}

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

// (...)

#endif

#endif //SUSHI_LV2_FEATURES_H