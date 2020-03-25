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

#include "lv2_worker.h"
#include "lv2_wrapper.h"

namespace sushi {
namespace lv2 {

/////////////////////////////////////////////////////////////////////////////////////////////////
// Static methods:
/////////////////////////////////////////////////////////////////////////////////////////////////

// With the Guitarix plugins, which are the only worker-thread plugins tested,
// This does not seem to be used - it never gets invoked, neither in Sushi nor in Jalv.
static LV2_Worker_Status lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
{
    auto worker = static_cast<Worker*>(handle);

    Lv2FifoItem response;
    response.size = size;

    // TODO: Is there a modern alternative to memcpy in this case?
    memcpy(response.block.data(), data, size);

    worker->responses().push(response);

    return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
{
    auto worker = static_cast<Worker*>(handle);
    auto model = worker->model();
    auto wrapper = model->wrapper();

    if (worker->threaded())
    {
        // Schedule a request to be executed by the worker thread

        Lv2FifoItem request;
        request.size = size;

        // TODO: Is there a modern alternative to memcpy in this case?
        memcpy(request.block.data(), data, size);

        worker->requests().push(request);

        auto e = RtEvent::make_async_work_event(&LV2_Wrapper::worker_callback,
                                                wrapper->id(),
                                                wrapper);

        auto id = e.async_work_event()->event_id();

        fprintf(stdout, "Worker event ID: %d\n", id);

        wrapper->set_pending_worker_event_id(id);

        wrapper->output_worker_event(e);
    }
    else
    {
        fprintf(stdout, "In lv2_worker_schedule immediately\n");

        // Execute work immediately in this thread
        std::unique_lock<std::mutex> lock(model->work_lock());

        worker->iface()->work(
                model->plugin_instance()->lv2_handle,
                lv2_worker_respond,
                worker,
                size,
                data);
    }
    return LV2_WORKER_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

Worker::Worker(Model* model)
{
    _model = model;
}

void Worker::init(const LV2_Worker_Interface* iface, bool threaded)
{
	_iface = iface;
	_threaded = threaded;
    _response.resize(4096);
}

void Worker::worker_func()
{
    if (_model->exit == true)
    {
        return;
    }

    std::unique_lock<std::mutex> lock(_model->work_lock());

    Lv2FifoItem request;

    _requests.pop(request);

    std::vector<std::byte> buf(request.size);

    // TODO: Is there a modern alternative to memcpy in this case?
    memcpy(buf.data(), request.block.data(), request.size);

    _iface->work(
            _model->plugin_instance()->lv2_handle,
            lv2_worker_respond,
            this,
            request.size,
            buf.data());
}


void Worker::emit_responses(LilvInstance* instance)
{
    while (_responses.empty() == false)
    {
        Lv2FifoItem response;

        _responses.pop(response);

        // TODO: Is there a modern alternative to memcpy in this case?
        memcpy(_response.data(), response.block.data(), response.size);

        _iface->work_response(instance->lv2_handle, response.size, _response.data());
    }
}

Model* Worker::model()
{
    return _model;
}

bool Worker::threaded()
{
    return _threaded;
}

Lv2WorkerFifo& Worker::requests()
{
    return _requests;
}

Lv2WorkerFifo& Worker::responses()
{
    return _responses;
}

const LV2_Worker_Interface* Worker::iface()
{
    return _iface;
}

}
}