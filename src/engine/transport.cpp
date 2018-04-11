#include <cassert>
#include <cmath>

#include "library/constants.h"
#include "transport.h"

namespace sushi {
namespace engine {

/* Calculate a multiplier factor detailing how to count quarter notes (beats)
 * based on the time signature. I.e 4/4 counts 4 beats to a bar (1-2-3-4) but
 * 6/8 would only count 2 (1 . . 2 . .)
 * TODO - incomplete and possibly wrong */
float calc_qn_multipler(TimeSignature signature)
{
    switch (signature.denominator)
    {
        case 2:
        case 4:
            switch (signature.numerator)
            {
                case 9:
                    return 3;
                default:
                    return 1;
            }
        case 8:
            switch (signature.numerator)
            {
                case 3:
                case 6:
                case 9:
                case 12:
                    return 3;
                default:
                    return 2;
            }
        default:
            return 1;
    }
}

void Transport::set_time(Time timestamp, int64_t samples)
{
    _time = timestamp + _latency;
    int64_t prev_samples = _sample_count;
    /* Assume that if there is a missed callback, the numbers of samples
     * will still be an even number of AUDIO_CHUNK_SIZE */
    auto chunks_passed = (samples - prev_samples) / AUDIO_CHUNK_SIZE;
    _sample_count = samples;

    _current_bar_qn_count +=  chunks_passed * _qns_per_chunk;
    if (_current_bar_qn_count > _qns_per_bar)
    {
        _current_bar_qn_count = std::fmod(_current_bar_qn_count, _qns_per_bar);
        _bar_start_qn_count += _qns_per_bar;
    }
    _qn_count += chunks_passed * _qns_per_chunk;
}

void Transport::set_time_signature(TimeSignature signature)
{
    _time_signature = signature;
    _update_coeffcients();
}

void Transport::set_tempo(float tempo)
{
    _tempo = tempo;
    _update_coeffcients();
}

void Transport::set_sample_rate(float sample_rate)
{
    _samplerate = sample_rate;
    _update_coeffcients();
}

void Transport::_update_coeffcients()
{
    assert(_samplerate > 0.0f);
    _qn_multiplier = calc_qn_multipler(_time_signature);
    _qns_per_chunk = 4.0 * static_cast<double>(AUDIO_CHUNK_SIZE) /
                     (static_cast<double>(_tempo) / 60.0 * _samplerate);
    _qns_per_bar = _time_signature.numerator / _qn_multiplier;
}





} // namespace engine
} // namespace sushi