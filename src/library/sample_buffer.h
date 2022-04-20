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

/**
 * @brief General purpose multichannel audio buffer class
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_SAMPLEBUFFER_H
#define SUSHI_SAMPLEBUFFER_H

#include <algorithm>
#include <cassert>
#include <cmath>

#include "constants.h"

namespace sushi {

constexpr int LEFT_CHANNEL_INDEX = 0;
constexpr int RIGHT_CHANNEL_INDEX = 1;

template<int size>
class SampleBuffer;

template<int size>
void swap(SampleBuffer<size>& lhs, SampleBuffer<size>& rhs)
{
    std::swap(lhs._channel_count, rhs._channel_count);
    std::swap(lhs._own_buffer, rhs._own_buffer);
    std::swap(lhs._buffer, rhs._buffer);
}

template<int size>
class SampleBuffer
{
public:
    /**
     * @brief Construct a zeroed buffer with specified number of channels
     */
    explicit SampleBuffer(int channel_count) : _channel_count(channel_count),
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
            if (_own_buffer && o._own_buffer)
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
                /* Assigning to or from a non owning buffer is only allowed if their
                 * channel count matches. In that case the underlying sample data is
                 * copied.
                 * If their sample counts differs this might trigger a (re)allocation
                 * of the internal data buffer, This would need to be resolved by
                 * either forcing the SampleBuffer owning the data to change its
                 * channel count or turn the non-owning SampleBuffer into a normal
                 * SampleBuffer that owns its data buffer, and hence losing the
                 * connection to the SampleBuffer that originally owned the data.
                 * Both of which will have unexpected, and most likely unwanted, side
                 * effects. */
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
    static SampleBuffer create_non_owning_buffer(SampleBuffer& source,
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
    static SampleBuffer create_non_owning_buffer(SampleBuffer& source)
    {
        return create_non_owning_buffer(source, 0, source.channel_count());
    }


    /**
     * @brief Create a Samplebuffer by wrapping a raw data pointer.
     *
     * @param data raw pointer to data stored in the same format of SampleBuffer storage
     * @param start_channel Index of first channel to wrap.
     * @param start_channel The first channel to wrap. Defaults to 0.
     * @param number_of_channels Must not exceed the channelcount of the source buffer
     *                           minus start_channel.
     * @return The created, non-owning SampleBuffer.
     */
    static SampleBuffer create_from_raw_pointer(float* data,
                                                int start_channel,
                                                int number_of_channels)
    {
        SampleBuffer buffer;
        buffer._own_buffer = false;
        buffer._channel_count = number_of_channels;
        buffer._buffer = data + size * start_channel;
        return buffer;
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
    void to_interleaved(float* interleaved_buf) const
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
     * @brief Replace the contents of the buffer with that of another buffer
     * @param source SampleBuffer with either 1 channel or the same number of
     *               channels as the destination buffer
     */
    void replace(const SampleBuffer &source)
    {
        assert(source.channel_count() == 1 || source.channel_count() == this->channel_count());

        if (source.channel_count() == 1) // mono input, copy to all dest channels
        {
            for (int channel = 0; channel < _channel_count; ++channel)
            {
                std::copy(source._buffer, source._buffer + size, _buffer + channel * size);
            }
        }
        else
        {
            std::copy(source._buffer, source._buffer + _channel_count * size, _buffer);
        }
    }

    /**
     * @brief Copy data channel by channel into this buffer from source buffer. No bounds checking.
     */
    void replace(int dest_channel, int source_channel, const SampleBuffer &source)
    {
        assert(source_channel < source.channel_count() && dest_channel < this->channel_count());
        std::copy(source.channel(source_channel),
                  source.channel(source_channel) + size,
                  _buffer + (dest_channel * size));
    }

    /**
     * @brief Sums the content of source into this buffer.
     * @param source SampleBuffer with either 1 channel or the same number of
     *               channels as the destination buffer
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
    void add(int dest_channel, int source_channel, const SampleBuffer& source)
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
     * @brief Sums one channel of source buffer into one channel of the buffer after applying gain.
     */
    void add_with_gain(int dest_channel, int source_channel, const SampleBuffer& source, float gain)
    {
        float* source_data = source._buffer + size * source_channel;
        float* dest_data = _buffer + size * dest_channel;
        for (int i = 0; i < size; ++i)
        {
            dest_data[i] += source_data[i] * gain;
        }
    }

    /**
     * @brief Sums the content of SampleBuffer source into this buffer after applying a
     *        linear gain ramp.
     *
     * @param source The buffer to copy from. Has to be either a 1 channel buffer or have
     *        the same number of channels as this buffer
     * @param start The value to start the ramp from
     * @param end The value to end the ramp
    */
    void add_with_ramp(const SampleBuffer &source, float start, float end)
    {
        assert(source.channel_count() == 1 || source.channel_count() == _channel_count);

        float inc = (end - start) / (size - 1);
        if (source.channel_count() == 1)
        {
            for (int channel = 0; channel < _channel_count; ++channel)
            {
                float* dest = _buffer + size * channel;
                for (int i = 0; i < size; ++i)
                {
                    dest[i] += source._buffer[i] * (start + i * inc);
                }
            }
        } else if (source.channel_count() == _channel_count)
        {
            for (int channel = 0; channel < _channel_count; ++channel)
            {
                float* source_data = _buffer + size * channel;
                float* dest_data = _buffer + size * channel;
                for (int i = 0; i < size; ++i)
                {
                    dest_data[i] += source_data[i] * (start + i * inc);
                }
            }
        }
    }

    /**
    * @brief Sums one channel of source buffer into one channel of the buffer after applying
    *        a linear gain ramp.
    */
    void add_with_ramp(int dest_channel, int source_channel, const SampleBuffer& source, float start, float end)
    {
        float inc = (end - start) / (size - 1);
        float* source_data = source._buffer + size * source_channel;
        float* dest_data = _buffer + size * dest_channel;
        for (int i = 0; i < size; ++i)
        {
            dest_data[i] += source_data[i] * (start + i * inc);
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
            float* data = _buffer + size * channel;
            for (int i = 0; i < size; ++i)
            {
                data[i] *= start + i * inc;
            }
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

    /**
     * @brief Count the number of samples outside of [-1.0, 1.0] in one channel
     * @param channel The channel to analyse, must not exceed the buffer's channelcount
     * @return The number of samples in the buffer whose absolute value is > 1.0
     */
    int count_clipped_samples(int channel) const
    {
        assert(channel < _channel_count);
        int clipcount = 0;
        const float* data = _buffer + size * channel;
        for (int i = 0 ; i < size; ++i)
        {
            /* std::abs() is more efficient than testing for upper and lower bound separately
               And GCC can compile this to vectorised, branchless code */
            clipcount += std::abs(data[i]) >= 1.0f;
        }
        return clipcount;
    }

    /**
     * @brief Calculate the peak value / loudest sample for one channel
     * @param channel The channel to analyse, must not exceed the buffer's channelcount
     * @return The absolute value of the loudest sample
     */
    float calc_peak_value(int channel) const
    {
        assert(channel < _channel_count);
        float max = 0.0f;
        const float* data = _buffer + size * channel;
        for (int i = 0 ; i < size; ++i)
        {
            max = std::max(max, std::abs(data[i]));
        }
        return max;
    }

    /**
     * @brief Calculate the root-mean-square average for one channel
     * @param channel The channel to analyse, must not exceed the buffer's channelcount
     * @return The RMS value of all the samples in the channel
     */
    float calc_rms_value(int channel) const
    {
        assert(channel < _channel_count);
        float sum = 0.0f;
        const float* data = _buffer + size * channel;
        for (int i = 0 ; i < size; ++i)
        {
            float s = data[i];
            sum += s * s;
        }
        return std::sqrt(sum / AUDIO_CHUNK_SIZE);
    }

private:
    int _channel_count;
    bool _own_buffer;
    float* _buffer;

    friend void swap<>(SampleBuffer<size>& lhs, SampleBuffer<size>& rhs);
};

typedef SampleBuffer<AUDIO_CHUNK_SIZE> ChunkSampleBuffer;
} // namespace sushi


#endif //SUSHI_SAMPLEBUFFER_H
