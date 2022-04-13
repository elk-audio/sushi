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

    memcpy(response.block.data(), data, size);

    worker->responses().push(response);

    return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status Worker::schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
{
    auto worker = static_cast<Worker*>(handle);
    auto model = worker->_model;
    auto wrapper = model->wrapper();

    if (worker->_threaded)
    {
        // Schedule a request to be executed by the worker thread

        Lv2FifoItem request;
        request.size = size;

        if (size > WORKER_REQUEST_SIZE)
        {
            return LV2_WORKER_ERR_NO_SPACE;
        }

        memcpy(request.block.data(), data, size);
        worker->requests().push(request);
        wrapper->request_worker_callback(&LV2_Wrapper::worker_callback);
    }
    else
    {
        // Execute work immediately in this thread
        std::unique_lock<std::mutex> lock(worker->_work_lock);

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
    _request.resize(4096);
    _response.resize(4096);
}

void Worker::worker_func()
{
    if (_model->exit == true)
    {
        return;
    }

    std::unique_lock<std::mutex> lock(_work_lock);

    Lv2FifoItem request;

    _requests.pop(request);

    memcpy(_request.data(), request.block.data(), request.size);

    _iface->work(
            _model->plugin_instance()->lv2_handle,
            lv2_worker_respond,
            this,
            request.size,
            _request.data());
}

void Worker::emit_responses(LilvInstance* instance)
{
    while (_responses.wasEmpty() == false)
    {
        Lv2FifoItem response;

        _responses.pop(response);

        memcpy(_response.data(), response.block.data(), response.size);

        _iface->work_response(instance->lv2_handle, response.size, _response.data());
    }
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
