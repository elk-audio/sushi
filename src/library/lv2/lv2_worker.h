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

#include "lv2/worker/worker.h"
#include "lv2_model.h"

namespace sushi {
namespace lv2 {

class Worker
{
public:
    Worker(Model* model);
    ~Worker();

    void init(const LV2_Worker_Interface* iface, bool threaded);

    void emit_responses(LilvInstance* instance);

    ZixSem sem;

    Model* model();

    bool threaded();

    ZixRing* requests();
    ZixRing* responses();

    const LV2_Worker_Interface* iface();

private:
    const LV2_Worker_Interface* _iface = nullptr; ///< Plugin worker interface

    // TODO: Introduce proper thread. std::thread
    ZixThread thread; ///< Worker thread

    ZixRing* _requests = nullptr; ///< Requests to the worker
    ZixRing* _responses = nullptr; ///< Responses from the worker

    void* _response = nullptr; ///< Worker response buffer

    void _finish();
    void _destroy();

    Model* _model{nullptr};
    bool _threaded{false};
};

LV2_Worker_Status lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);



}
}