/**
 * @Brief General purpose multichannel audio buffer class
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_SAMPLEBUFFER_H
#define SUSHI_SAMPLEBUFFER_H

#include <cstring>
#include <algorithm>
#include <cassert>

#include "constants.h"

namespace sushi {

template<int size>
class SampleBuffer
{
public:
    /**
     * @brief Construct a zeroed buffer with specified number of channels
     */
    SampleBuffer(int channel_count) : _channel_count(channel_count),
                                      _own_buffer(true),
                                      _buffer(new float[size * channel_count])
    {
        clear();
    }

    /**
     * @brief Construct an empty buffer object with 0 channels.
     */
    SampleBuffer() noexcept : _channel_count(0),
                              _own_buffer(true),
                              _buffer(nullptr)
    {}


    /**
     * @brief Copy constructor.
     */
    SampleBuffer(const SampleBuffer &o) : _channel_count(o._channel_count),
                                          _own_buffer(o._own_buffer)
    {
        if (o._own_buffer)
        {
            _buffer = new float[size * o._channel_count];
            std::copy(o._buffer, o._buffer + (size * o._channel_count), _buffer);
        } else
        {
            _buffer = o._buffer;
        }
    }


    /**
     * @brief Move constructor.
     */
    SampleBuffer(SampleBuffer &&o) noexcept : _channel_count(o._channel_count),
                                              _own_buffer(o._own_buffer),
                                              _buffer(o._buffer)
    {
        o._buffer = nullptr;
    }


    /**
     * @brief Destroy the buffer.
     */
    ~SampleBuffer()
    {
        if (_own_buffer)
        {
            delete[] _buffer;
        }
    }


    /**
     * @brief Assign to this buffer.
     */
    SampleBuffer &operator=(const SampleBuffer &o)
    {
        if (this != &o)  // Avoid self-assignment
        {
            if (_own_buffer)
            {
                if (_channel_count != o._channel_count)
                {
                    delete[] _buffer;
                    _buffer = (o._channel_count > 0)? (new float[size * o._channel_count]) : nullptr;
                    _channel_count = o._channel_count;
                }
            }
            else
            {
                /* TODO - Consider what should happen if you assign to a non-owning
                 * buffer with a buffer with different number of channels.
                 * Currently we disallow this by an assert.
                 * Perhaps this scenario should trigger a re-allocation of the buffer,
                 * memory that the SampleBuffer then takes ownership of. But that
                 * solution also loses the connection to the buffer originally
                 * owning the data buffer. */
                assert(_channel_count == o._channel_count);
            }
            std::copy(o._buffer, o._buffer + (size * o._channel_count), _buffer);
        }
        return *this;
    }


    /**
     * @brief Assign to this buffer using move semantics.
     */
    SampleBuffer &operator=(SampleBuffer &&o) noexcept
    {
        if (this != &o)  // Avoid self-assignment
        {
            if (_own_buffer)
            {
                delete[] _buffer;
            }
            _channel_count = o._channel_count;
            _own_buffer = o._own_buffer;
            _buffer = o._buffer;
            o._buffer = nullptr;
        }
        return *this;
    }

    /**
     * @brief Create a SampleBuffer from another SampleBuffer, without copying or
     *        taking ownership of the data. Optionally only a subset of
     *        the source buffers channels can be wrapped.
     * @param source The SampleBuffer whose data is wrapped.
     * @param start_channel The first channel to wrap. Defaults to 0.
     * @param number_of_channels Must not exceed the channelcount of the source buffer
     *                           minus start_channel.
     * @return The created, non-owning SampleBuffer.
     */
    static SampleBuffer create_non_owning_buffer(const SampleBuffer& source,
                                                 int start_channel,
                                                 int number_of_channels)
    {
        assert(number_of_channels + start_channel <= source._channel_count);

        SampleBuffer buffer;
        buffer._own_buffer = false;
        buffer._channel_count = number_of_channels;
        buffer._buffer = source._buffer + size * start_channel;
        return buffer;
    }

    /**
     * @brief Defaulted version of the above function.
     */
    static SampleBuffer create_non_owning_buffer(const SampleBuffer& source)
    {
        return create_non_owning_buffer(source, 0, source.channel_count());
    }

    /**
     * @brief Zero the entire buffer
     */
    void clear()
    {
        std::fill(_buffer, _buffer + (size * _channel_count), 0.0f);
    }

    /**
    * @brief Returns a writeable pointer to a specific channel in the buffer. No bounds checking.
    */
    float* channel(int channel)
    {
        return _buffer + channel * size;
    }

    /**
    * @brief Returns a read-only pointer to a specific channel in the buffer. No bounds checking.
    */
    const float* channel(int channel) const
    {
        return _buffer + channel * size;
    }

    /**
     * @brief Gets the number of channels in the buffer.
     */
    int channel_count() const
    {
        return _channel_count;
    }

    /**
     * @brief Copy interleaved audio data from interleaved_buf to this buffer.
     */
    void from_interleaved(const float* interleaved_buf)
    {
        switch (_channel_count)
        {
            case 2:  // Most common case, others are mostly included for future compatibility
            {
                float* l_in = _buffer;
                float* r_in = _buffer + size;
                for (int n = 0; n < size; ++n)
                {
                    *l_in++ = *interleaved_buf++;
                    *r_in++ = *interleaved_buf++;
                }
                break;
            }
            case 1:
            {
                std::copy(interleaved_buf, interleaved_buf + size, _buffer);
                break;
            }
            default:
            {
                for (int n = 0; n < size; ++n)
                {
                    for (int c = 0; c < _channel_count; ++c)
                    {
                        _buffer[n + c * _channel_count] = *interleaved_buf++;
                    }
                }
            }
        }
    }

    /**
     * @brief Copy buffer data in interleaved format to interleaved_buf
     */
    void to_interleaved(float* interleaved_buf)
    {
        switch (_channel_count)
        {
            case 2:  // Most common case, others are mostly included for future compatibility
            {
                float* l_out = _buffer;
                float* r_out = _buffer + size;
                for (int n = 0; n < size; ++n)
                {
                    *interleaved_buf++ = *l_out++;
                    *interleaved_buf++ = *r_out++;
                }
                break;
            }
            case 1:
            {
                std::copy(_buffer, _buffer + size, interleaved_buf);
                break;
            }
            default:
            {
                for (int n = 0; n < size; ++n)
                {
                    for (int c = 0; c < _channel_count; ++c)
                    {
                        *interleaved_buf++ = _buffer[n + c * size];
                    }
                }
            }
        }
    }

    /**
     * @brief Apply a fixed gain to the entire buffer.
     */
    void apply_gain(float gain)
    {
        for (int i = 0; i < size * _channel_count; ++i)
        {
            _buffer[i] *= gain;
        }
    }

    /**
    * @brief Apply a fixed gain to a given channel.
    */
    void apply_gain(float gain, int channel)
    {
        float* data = _buffer + size * channel;
        for (int i = 0; i < size; ++i)
        {
            data[i] *= gain;
        }
    }

    /**
     * @brief Copy data channel by channel into this buffer from source buffer. No bounds checking.
     */
    void replace(int dest_channel, int source_channel, SampleBuffer &source)
    {
        std::copy(source.channel(source_channel),
                  source.channel(source_channel) + size,
                  _buffer + (dest_channel * size));
    }

    /**
     * @brief Sums the content of source into this buffer.
     *
     * source has to be either a 1 channel buffer or have the same number of channels
     * as the destination buffer.
     */
    void add(const SampleBuffer &source)
    {
        assert(source.channel_count() == 1 || source.channel_count() == this->channel_count());

        if (source.channel_count() == 1) // mono input, add to all dest channels
        {
            for (int channel = 0; channel < _channel_count; ++channel)
            {
                float* dest = _buffer + size * channel;
                for (int i = 0; i < size; ++i)
                {
                    dest[i] += source._buffer[i];
                }
            }
        } else if (source.channel_count() == _channel_count)
        {
            for (int i = 0; i < size * _channel_count; ++i)
            {
                _buffer[i] += source._buffer[i];
            }
        }
    }

    /**
     * @brief Sums one channel of source buffer into one channel of the buffer.
     */
    void add(int dest_channel, int source_channel, SampleBuffer &source)
    {
        float* source_data = source._buffer + size * source_channel;
        float* dest_data = _buffer + size * dest_channel;
        for (int i = 0; i < size; ++i)
        {
            dest_data[i] += source_data[i];
        }
    }

    /**
     * @brief Sums the content of SampleBuffer source into this buffer after applying a gain.
     *
     * source has to be either a 1 channel buffer or have the same number of channels
     * as the destination buffer.
    */
    void add_with_gain(const SampleBuffer &source, float gain)
    {
        assert(source.channel_count() == 1 || source.channel_count() == this->channel_count());

        if (source.channel_count() == 1)
        {
            for (int channel = 0; channel < _channel_count; ++channel)
            {
                float* dest = _buffer + size * channel;
                for (int i = 0; i < size; ++i)
                {
                    dest[i] += source._buffer[i] * gain;
                }
            }
        } else if (source.channel_count() == _channel_count)
        {
            for (int i = 0; i < size * _channel_count; ++i)
            {
                _buffer[i] += source._buffer[i] * gain;
            }
        }
    }

    /**
     * @brief Ramp the volume of all channels linearly from start to end
     * @param start The value to start the ramp from
     * @param end The value to end the ramp
     */
    void ramp(float start, float end)
    {
        float inc = (end - start) / (size - 1);
        for (int channel = 0; channel < _channel_count; ++channel)
        {
            float vol = start;
            float* data = _buffer + size * channel;
            for (int i = 0; i < size - 1; ++i)
            {
                data[i] *= vol;
                vol += inc;
            }
            data[size - 1] *= end; /* Neutralize rounding errors */
        }
    }

    /**
     * @brief Convenience wrapper for ramping up from 0 to unity
     */
    void ramp_up()
    {
        this->ramp(0.0f, 1.0f);
    }

    /**
     * @brief Convenience wrapper for ramping from unity down to 0
     */
    void ramp_down()
    {
        this->ramp(1.0f, 0.0f);
    }


private:
    int _channel_count;
    bool _own_buffer;
    float* _buffer;
};

typedef SampleBuffer<AUDIO_CHUNK_SIZE> ChunkSampleBuffer;
} // namespace sushi


#endif //SUSHI_SAMPLEBUFFER_H
