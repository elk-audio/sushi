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

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"
#include "lv2/worker/worker.h"
#include "lv2_model.h"

namespace sushi {
namespace lv2 {

constexpr int WORKER_FIFO_SIZE = 128;
constexpr int WORKER_REQUEST_SIZE = 64;

struct Lv2FifoItem
{
    int size{0};

    // The zix ring buffer does not enforce a fixed block size.
    // Instead, it is 4096 bytes in total for the buffer.
    // Each entry is a size uint, followed by as many bytes as defined in that.
    // So the safest thing would be 4096-4 really.
    std::array<std::byte, WORKER_REQUEST_SIZE> block;
};

using Lv2WorkerFifo = memory_relaxed_aquire_release::CircularFifo<Lv2FifoItem, WORKER_FIFO_SIZE>;

class Worker
{
public:
    explicit Worker(Model* model);
    ~Worker() = default;

    void init(const LV2_Worker_Interface* iface, bool threaded);

    void emit_responses(LilvInstance* instance);

    Lv2WorkerFifo& requests();

    Lv2WorkerFifo& responses();

    const LV2_Worker_Interface* iface();

    void worker_func();

    // Signature is as required by LV2 api.
    static LV2_Worker_Status schedule(LV2_Worker_Schedule_Handle handle, uint32_t size, const void *data);

private:
    std::mutex _work_lock;

    const LV2_Worker_Interface* _iface = nullptr;

    Lv2WorkerFifo _requests;
    Lv2WorkerFifo _responses;

    std::vector<std::byte> _request;
    std::vector<std::byte> _response;

    Model* _model{nullptr};
    bool _threaded{false};
};

}
}

#endif //SUSHI_BUILD_WITH_LV2

#endif //SUSHI_LV2_WORKER_H