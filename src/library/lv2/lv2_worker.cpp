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

namespace sushi {
namespace lv2 {

static LV2_Worker_Status lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
{
    auto worker = static_cast<Worker*>(handle);
    zix_ring_write(worker->responses(), (const char*)&size, sizeof(size));
    zix_ring_write(worker->responses(), (const char*)data, size);
    fprintf(stdout, "In lv2_worker_respond\n");
    return LV2_WORKER_SUCCESS;
}

void Worker::worker_func()
{
    fprintf(stdout, "In worker_func\n");

    void* buf = nullptr;
    while (true)
    {
        fprintf(stdout, "In worker_func - before wait\n");
        zix_sem_wait(&sem);

        fprintf(stdout, "In worker_func - after wait\n");
        if (_model->exit == true)
        {
            fprintf(stdout, "In worker_func - breaking for exit\n");
            break;
        }

        fprintf(stdout, "In worker_func - after exit block\n");

        uint32_t size = 0;
        zix_ring_read(_requests, (char*)&size, sizeof(size));

        fprintf(stdout, "In worker_func - after reading request size: %d\n", size);

        if (!(buf = realloc(buf, size)))
        {
            fprintf(stderr, "error: realloc() failed\n");
            free(buf);
        }

        zix_ring_read(_requests, (char*)buf, size);

        fprintf(stdout, "In worker_func - after reading request.\n");

        // It seems to wait here forever!
        zix_sem_wait(&_model->work_lock);

        _iface->work(
                _model->plugin_instance()->lv2_handle,
                lv2_worker_respond,
                this,
                size,
                buf);

        fprintf(stdout, "In worker_func - after waiting on work lock!!!\n");

        zix_sem_post(&_model->work_lock);
    }

    free(buf);
}

LV2_Worker_Status lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
{
    auto worker = static_cast<Worker*>(handle);
    auto model = worker->model();

    if (worker->threaded())
    {
        fprintf(stdout, "In lv2_worker_schedule threaded\n");
        // Schedule a request to be executed by the worker thread
        zix_ring_write(worker->requests(), (const char*)&size, sizeof(size));
        zix_ring_write(worker->requests(), (const char*)data, size);
        zix_sem_post(&worker->sem);
    }
    else
    {
        fprintf(stdout, "In lv2_worker_schedule immediately\n");
        // Execute work immediately in this thread
        zix_sem_wait(&model->work_lock);
        worker->iface()->work(
                model->plugin_instance()->lv2_handle, lv2_worker_respond, worker, size, data);
        zix_sem_post(&model->work_lock);
    }
    return LV2_WORKER_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

Worker::Worker(Model* model)
{
    _model = model;
}

Worker::~Worker()
{
    _finish();
    _destroy();
}

// TODO: what is ZIX_UNUSED?
void Worker::init(const LV2_Worker_Interface* iface, bool threaded)
{
	_iface = iface;
	_threaded = threaded;

    fprintf(stdout, "In init\n");

	if (_threaded)
	{
        _thread = std::thread(&Worker::worker_func, this);

		_requests = zix_ring_new(4096);
		zix_ring_mlock(_requests);
	}

	_responses = zix_ring_new(4096);
	_response  = malloc(4096);
	zix_ring_mlock(_responses);
}

void Worker::_finish()
{
    fprintf(stdout, "In finish\n");
	if (_threaded)
	{
		zix_sem_post(&sem);
        _thread.join();
	}
}

void Worker::_destroy()
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

void Worker::emit_responses(LilvInstance* instance)
{
	if (_responses)
	{
		uint32_t read_space = zix_ring_read_space(_responses);
		while (read_space)
		{
            fprintf(stdout, "In emit_responses\n");
			uint32_t size = 0;
			zix_ring_read(_responses, (char*)&size, sizeof(size));

			zix_ring_read(_responses, (char*)_response, size);

			_iface->work_response(
				instance->lv2_handle, size, _response);

			read_space -= sizeof(size) + size;
		}
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

ZixRing* Worker::requests()
{
    return _requests;
}

ZixRing* Worker::responses()
{
    return _responses;
}

const LV2_Worker_Interface* Worker::iface()
{
    return _iface;
}

}
}