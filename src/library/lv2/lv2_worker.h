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

#ifndef SUSHI_LV2_WORKER_H
#define SUSHI_LV2_WORKER_H

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2/worker/worker.h"
#include "lv2_model.h"
#include "lv2_worker_fifo.h"

namespace sushi {
namespace lv2 {

class Worker
{
public:
    Worker(Model* model);
    ~Worker() = default;

    void init(const LV2_Worker_Interface* iface, bool threaded);

    void emit_responses(LilvInstance* instance);

    Model* model();

    bool threaded();

    Lv2WorkerFifo& requests();

    Lv2WorkerFifo& responses();

    const LV2_Worker_Interface* iface();

    void worker_func();

private:
    const LV2_Worker_Interface* _iface = nullptr;

    Lv2WorkerFifo _requests;
    Lv2WorkerFifo _responses;

    std::vector<std::byte> _response;

    Model* _model{nullptr};
    bool _threaded{false};
};

LV2_Worker_Status lv2_worker_schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);

}
}

#endif //SUSHI_BUILD_WITH_LV2

#endif //SUSHI_LV2_WORKER_H