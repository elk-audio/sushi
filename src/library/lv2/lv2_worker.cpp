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

#include "zix/ring.h"
#include "zix/sem.h"
#include "zix/thread.h"

namespace sushi {
namespace lv2 {

static LV2_Worker_Status lv2_worker_respond(LV2_Worker_Respond_Handle handle, uint32_t size, const void* data)
{
    Lv2_Worker* worker = (Lv2_Worker*)handle;
	zix_ring_write(worker->responses, (const char*)&size, sizeof(size));
	zix_ring_write(worker->responses, (const char*)data, size);
    fprintf(stdout, "In lv2_worker_respond\n");
	return LV2_WORKER_SUCCESS;
}

static void* worker_func(void* data)
{
    Lv2_Worker* worker = (Lv2_Worker*)data;
    Model* model = worker->model;

    fprintf(stdout, "In worker_func\n");

	void* buf = NULL;
	while (true)
	{
        fprintf(stdout, "In worker_func - before wait\n");
		zix_sem_wait(&worker->sem);

        fprintf(stdout, "In worker_func - after wait\n");
		if (model->lv2_exit == true)
		{
            fprintf(stdout, "In worker_func - breaking for exit\n");
			break;
		}

        fprintf(stdout, "In worker_func - after exit block\n");

		uint32_t size = 0;
		zix_ring_read(worker->requests, (char*)&size, sizeof(size));

        fprintf(stdout, "In worker_func - after reading request size: %d\n", size);

		if (!(buf = realloc(buf, size)))
		{
			fprintf(stderr, "error: realloc() failed\n");
			free(buf);
			return NULL;
		}

		zix_ring_read(worker->requests, (char*)buf, size);

        fprintf(stdout, "In worker_func - after reading request.\n");

        // It seems to wait here forever!
		zix_sem_wait(&model->work_lock);

		worker->iface->work(
			model->plugin_instance()->lv2_handle,
			lv2_worker_respond,
			worker,
			size,
			buf);

        fprintf(stdout, "In worker_func - after waiting on work lock!!!\n");

		zix_sem_post(&model->work_lock);
	}

	free(buf);

	return NULL;
}

// TODO: what is ZIX_UNUSED?
void lv2_worker_init(ZIX_UNUSED Model* model, Lv2_Worker* worker, const LV2_Worker_Interface* iface, bool threaded)
{
	worker->iface = iface;
	worker->threaded = threaded;

    fprintf(stdout, "In lv2_worker_init\n");

	if (threaded)
	{
		zix_thread_create(&worker->thread, 4096, worker_func, worker);
		worker->requests = zix_ring_new(4096);
		zix_ring_mlock(worker->requests);
	}


	worker->responses = zix_ring_new(4096);
	worker->response  = malloc(4096);
	zix_ring_mlock(worker->responses);
}

void lv2_worker_finish(Lv2_Worker* worker)
{
    fprintf(stdout, "In lv2_worker_finish\n");
	if (worker->threaded)
	{
		zix_sem_post(&worker->sem);
		zix_thread_join(worker->thread, NULL);
	}
}

void lv2_worker_destroy(Lv2_Worker* worker)
{
	if (worker->requests)
	{
		if (worker->threaded)
		{
			zix_ring_free(worker->requests);
		}

		if(worker->responses)
		{
            zix_ring_free(worker->responses);
            free(worker->response);
        }
	}
}

LV2_Worker_Status lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void* data)
{
    Lv2_Worker* worker = (Lv2_Worker*)handle;
    Model* model = worker->model;

	if (worker->threaded)
	{
        fprintf(stdout, "In lv2_worker_schedule threaded\n");
		// Schedule a request to be executed by the worker thread
		zix_ring_write(worker->requests, (const char*)&size, sizeof(size));
		zix_ring_write(worker->requests, (const char*)data, size);
		zix_sem_post(&worker->sem);
	}
	else
	{
        fprintf(stdout, "In lv2_worker_schedule immediately\n");
		// Execute work immediately in this thread
		zix_sem_wait(&model->work_lock);
		worker->iface->work(
                model->plugin_instance()->lv2_handle, lv2_worker_respond, worker, size, data);
		zix_sem_post(&model->work_lock);
	}
	return LV2_WORKER_SUCCESS;
}

void lv2_worker_emit_responses(Lv2_Worker* worker, LilvInstance* instance)
{
	if (worker->responses)
	{
		uint32_t read_space = zix_ring_read_space(worker->responses);
		while (read_space)
		{
            fprintf(stdout, "In lv2_worker_emit_responses\n");
			uint32_t size = 0;
			zix_ring_read(worker->responses, (char*)&size, sizeof(size));

			zix_ring_read(worker->responses, (char*)worker->response, size);

			worker->iface->work_response(
				instance->lv2_handle, size, worker->response);

			read_space -= sizeof(size) + size;
		}
	}
}

}
}