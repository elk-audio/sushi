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
#include "lv2_model.h"

namespace sushi {
namespace lv2 {

static LV2_Worker_Status lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
{
    auto worker = static_cast<Lv2_Worker*>(handle);
	zix_ring_write(worker->get_responses(), (const char*)&size, sizeof(size));
	zix_ring_write(worker->get_responses(), (const char*)data, size);
	return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
{
    auto worker = static_cast<Lv2_Worker*>(handle);
    auto model = worker->get_model();

    if (worker->is_threaded())
    {
        // Schedule a request to be executed by the worker thread
        zix_ring_write(worker->get_requests(), (const char*)&size, sizeof(size));
        zix_ring_write(worker->get_requests(), (const char*)data, size);
        worker->get_semaphore().notify();
    }
    else
    {
        // Execute work immediately in this thread

        std::unique_lock<std::mutex> lock(model->get_work_lock());
        worker->get_iface()->work(
                model->get_plugin_instance()->lv2_handle, lv2_worker_respond, worker, size, data);
    }
    return LV2_WORKER_SUCCESS;
}

void Lv2_Worker::worker_func()
{
	void* buf = nullptr;
	while (true)
	{
        _semaphore.wait();

		if (_model->get_exit())
		{
			break;
		}

		uint32_t size = 0;
		zix_ring_read(_requests, (char*)&size, sizeof(size));

		if (!(buf = realloc(buf, size)))
		{
			fprintf(stderr, "error: realloc() failed\n");
			free(buf);
		}

		zix_ring_read(_requests, (char*)buf, size);

        std::unique_lock<std::mutex> lock(_model->get_work_lock());
		_iface->work(_model->get_plugin_instance()->lv2_handle, lv2_worker_respond, this, size, buf);
	}

	free(buf);
}

Lv2_Worker::Lv2_Worker(LV2Model* model, bool threaded)
{
    _model = model;

	_threaded = threaded;

	if (_threaded)
	{
        _thread = std::thread(&Lv2_Worker::worker_func, this);

        _requests = zix_ring_new(4096);
		zix_ring_mlock(_requests);
	}

    _responses = zix_ring_new(4096);
    _response  = malloc(4096);
	zix_ring_mlock(_responses);
}

Lv2_Worker::~Lv2_Worker()
{
    finish();
    destroy();
}

void Lv2_Worker::finish()
{
	if (_threaded)
	{
        _semaphore.notify();
        _thread.join();
    }
}

void Lv2_Worker::destroy()
{
	if (_requests)
	{
		if (_threaded)
		{
			zix_ring_free(_requests);
		}

		if(_responses)
		{
            zix_ring_free(_responses);
            free(_response);
        }
	}
}

void Lv2_Worker::emit_responses(LilvInstance* instance)
{
    if (_responses)
    {
        uint32_t read_space = zix_ring_read_space(_responses);
        while (read_space)
        {
            uint32_t size = 0;
            zix_ring_read(_responses, (char*)&size, sizeof(size));

            zix_ring_read(_responses, (char*)_response, size);

            _iface->work_response(instance->lv2_handle, size, _response);

            read_space -= sizeof(size) + size;
        }
    }
}

ZixRing* Lv2_Worker::get_requests()
{
    return _requests;
}

ZixRing* Lv2_Worker::get_responses()
{
    return _responses;
}

const LV2_Worker_Interface* Lv2_Worker::get_iface()
{
    return _iface;
}

void Lv2_Worker::set_iface(const LV2_Worker_Interface* iface)
{
    _iface = iface;
}

bool Lv2_Worker::is_threaded()
{
    return _threaded;
}

LV2Model* Lv2_Worker::get_model()
{
    return _model;
}

Semaphore& Lv2_Worker::get_semaphore()
{
    return _semaphore;
}

}
}