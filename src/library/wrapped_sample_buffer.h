/**
 * @Brief Non-owning version af a eneral purpose multichannel audio buffer
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Use it to wrap sample data in a buffer without taking ownership of the actual data.
 * Useful for instance in splitting multichannel buffers in individual channels
 */
#ifndef SUSHI_WRAPPED_SAMPLE_BUFFER_H
#define SUSHI_WRAPPED_SAMPLE_BUFFER_H
#include "library/sample_buffer.h"

namespace sushi {

template<int size>
class WrappedSampleBuffer : public SampleBuffer<size>
{
public:
    /**
     * @brief Construct an empty buffer object with 0 channels.
     */
    /*WrappedSampleBuffer() noexcept : SampleBuffer<size>::_channel_count(0),
                                     SampleBuffer<size>::_buffer(nullptr)
    {};*/

    /**
     * @brief Construct a buffer that wraps the data in selected number of channels in source_buffer.
     */
    WrappedSampleBuffer(SampleBuffer<size>& source_buffer, int source_channel = 0, int channel_count = 0) noexcept /*:
            SampleBuffer<size>::_channel_count(channel_count),
            SampleBuffer<size>::_buffer(source_buffer.channel(source_channel))*/
    {
        SampleBuffer<size>::_buffer = source_buffer.channel(source_channel);
        if (channel_count == 0)
        {
            SampleBuffer<size>::_channel_count = source_buffer.channel_count();
        } else
        {
            SampleBuffer<size>::_channel_count = channel_count;
        }
    };

    ~WrappedSampleBuffer()
    {
        // TODO - Find a better way of avoid calling the base class constructor all together
        SampleBuffer<size>::_buffer = nullptr;
    };
};

} //namspace sushi
#endif //SUSHI_WRAPPED_SAMPLE_BUFFER_H
