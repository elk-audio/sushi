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

#include "plugin_interface.h"

template<unsigned int size=AUDIO_CHUNK_SIZE>
class SampleBuffer
{
public:
    SampleBuffer(unsigned int channel_count) : _channel_count(channel_count),
                                               _buffer(new float[size * channel_count])
    {
        clear();
    }

    SampleBuffer() : _channel_count(0),
                     _buffer(nullptr)
    {
    }

    // Copy constructor
    SampleBuffer(const SampleBuffer& o) : _channel_count(o._channel_count),
                                          _buffer(new float[size * o._channel_count])
    {
        std::copy(o._buffer, o._buffer + (size * o._channel_count), _buffer);
    }

    // Copy Constructor that takes r-values
    SampleBuffer(SampleBuffer&& o) noexcept : _channel_count(o._channel_count),
                                              _buffer(o._buffer)
    {
        o._buffer = nullptr;
    }

    // Destructor
    ~SampleBuffer()
    {
        delete[] _buffer;
    }

    // Copy assignment operator
    SampleBuffer& operator = (const SampleBuffer& o)
    {
        if (this != &o)  // Avoid self-assignment
        {
            if (_channel_count != o._channel_count && o._channel_count != 0)
            {
                delete[] _buffer;
                _buffer = new float[size * o._channel_count];
            }
            _channel_count = o._channel_count;
            std::copy(o._buffer, o._buffer + (size * o._channel_count), _buffer);
        }
        return *this;
    }

    // copy assignment for r-values
    SampleBuffer& operator = (SampleBuffer&& o) noexcept
    {
        if (this != &o)  // Doubtful if neccesary
        {
            delete[] _buffer;
            _channel_count = o._channel_count;
            _buffer = o._buffer;
            o._buffer = nullptr;
        }
        return *this;
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
    float* channel(unsigned int channel)
    {
        return _buffer + channel * size;
    }

    /**
    * @brief Returns a read-only pointer to a specific channel in the buffer. No bounds checking.
    */
    /*const float* channel(unsigned int channel)
    {
        return _buffer + channel * size;
    }*/

    /**
     * @brief Gets the number of channels in the buffer.
     */
    unsigned int channel_count()
    {
        return _channel_count;
    }

    /**
     * @brief Copy interleaved audio data from interleaved_buf to this buffer.
     */
    void input_from_interleaved(const float *interleaved_buf)
    {
        switch (_channel_count)
        {
            case 2:  // Most common case, others are mostly included for future compatibility
            {
                float* l_in = _buffer;
                float* r_in = _buffer + size;
                for (unsigned int n = 0; n < size; ++n)
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
                for (unsigned int n = 0; n < size; ++n)
                {
                    for (unsigned int c = 0; c < _channel_count; ++c)
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
    void output_to_interleaved(float *interleaved_buf)
    {
        switch (_channel_count)
        {
            case 2:  // Most common case, others are mostly included for future compatibility
            {
                float* l_out = _buffer;
                float* r_out = _buffer + size;
                for (unsigned int n = 0; n < size; ++n)
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
                for (unsigned int n = 0; n < size; ++n)
                {
                    for (unsigned int c = 0; c < _channel_count; ++c)
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

    }


//private:
    unsigned int _channel_count;
    float* _buffer;
};

typedef SampleBuffer<2> SampleChunkBuffer;

#endif //SUSHI_SAMPLEBUFFER_H
