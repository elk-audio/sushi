/**
 * @brief Simple, non-thread safe fifo queue for use internally in the engine when
 *        concurrent access is not neccesary.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SIMPLE_FIFO_H
#define SUSHI_SIMPLE_FIFO_H

#include <array>

#include "library/rt_event_pipe.h"

namespace sushi {

template<typename T, size_t capacity>
class SimpleFifo
{
public:
    bool push(const T& element)
    {
        int new_tail = increment(_tail);
        if (new_tail == _head)
        {
            return false; // Fifo is full
        }
        _data[_tail] = element;
        _tail = new_tail;
        return true;
    }

    bool pop(T& element)
    {
        if (empty())
        {
            return false; // Fifo is empty
        }
        element = _data[_head];
        _head = increment(_head);
        return true;
    }

    const T& pop()
    {
        // This should not be called on an empty queue
        assert(empty() == false);
        int old_head = _head;
        _head = increment(_head);
        return _data[old_head];
    }

    T operator [](int i) const
    {
        assert(static_cast<size_t>(i) < capacity);
        return _data[(_head + i) % capacity];
    }

    T& operator [](int i)
    {
        assert(static_cast<size_t>(i) < capacity);
        return _data[(_head + i) % capacity];
    }

    int size() const
    {
        if (_head <= _tail)
        {
            return _tail - _head;
        }
        return capacity - _head + _tail;
    }

    int capacity_() const {return capacity;}

    bool empty() const {return _head == _tail;}

    void clear()
    {
        _head = 0;
        _tail = 0;
    }

private:
    inline int increment(int index) {return (index + 1) % capacity;}

    std::array<T, capacity> _data;
    // Elements are pushed at the tail and read from the head (head is first in the queue)
    int _head{0};
    int _tail{0};
};

template <size_t size>
class NonRtSafeRtEventFifo : public SimpleFifo<RtEvent, size>, public RtEventPipe
{
    NonRtSafeRtEventFifo() = default;
    virtual ~NonRtSafeRtEventFifo() = default;

    void send_event(const RtEvent &event) override {SimpleFifo<RtEvent, size>::push(event);}
};

}// namespace sushi

#endif //SUSHI_SIMPLE_FIFO_H
