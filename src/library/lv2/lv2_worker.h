/*
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef SUSHI_LV2_WORKER_H
#define SUSHI_LV2_WORKER_H

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2/worker/worker.h"

#include "lv2_semaphore.h"
#include "zix/ring.h"
#include "zix/thread.h"
#include <lilv-0/lilv/lilv.h>

namespace sushi {
namespace lv2 {

class LV2Model;

class Lv2_Worker
{
public:
    Lv2_Worker(LV2Model* model, bool threaded);
    ~Lv2_Worker();

    void finish();

    void destroy();

    void emit_responses(LilvInstance* instance);

    ZixRing* get_requests();
    ZixRing* get_responses();

    const LV2_Worker_Interface* get_iface();
    void set_iface(const LV2_Worker_Interface* iface);

    bool is_threaded();

    LV2Model* get_model();

    Semaphore& get_semaphore();

private:
    LV2Model* _model;
    Semaphore _semaphore;

    ZixRing* _requests = nullptr; ///< Requests to the worker
    ZixRing* _responses = nullptr; ///< Responses from the worker

// TODO: Introduce std::thread instead - to remove Zix dependency.
    ZixThread _thread; ///< Worker thread

    void* _response = nullptr; ///< Worker response buffer

    const LV2_Worker_Interface* _iface = nullptr; ///< Plugin worker interface
    bool _threaded = false; ///< Run work in another thread
};

LV2_Worker_Status lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);

}
}

#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

#include "library/processor.h"

namespace sushi {
namespace lv2 {
/* If LV2 support is disabled in the build, the wrapper is replaced with this
   minimal dummy processor whose purpose is to log an error message if a user
   tries to load an LV2 plugin */
class Lv2Wrapper : public Processor
{
public:
    Lv2Wrapper(HostControl host_control, const std::string& /*lv2_plugin_uri*/) :
        Processor(host_control) {}

    ProcessorReturnCode init(float sample_rate) override
    {
        return ProcessorReturnCode::ERROR;
    }

    void process_event(RtEvent /*event*/) override {}
    void process_audio(const ChunkSampleBuffer& /*in*/, ChunkSampleBuffer& /*out*/) override {}
};

}// end namespace lv2
}// end namespace sushi
#endif

#endif //SUSHI_LV2_WORKER_H