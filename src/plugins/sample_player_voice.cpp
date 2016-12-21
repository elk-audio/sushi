#include <cassert>
#include <cmath>
#include <iostream>

#include "sample_player_voice.h"

namespace sample_player_voice {

float Sample::at(float position)
{
    assert(position >= 0);

    float pos_int = std::floor(position);
    float weight = position - pos_int;
    int sample_pos = static_cast<int>(pos_int);

    float sample_low = (sample_pos < _length) ? _data[sample_pos] : 0.0f;
    float sample_high = (sample_pos + 1 < _length) ? _data[sample_pos + 1] : 0.0f;

    return (sample_high * weight + sample_low * (1 - weight));
}



}// namespace sample_player_plugin