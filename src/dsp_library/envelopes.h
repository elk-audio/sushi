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
 * @brief Generic envelope classes to be used as building blocks for audio processors
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_ENVELOPES_H
#define SUSHI_ENVELOPES_H

#include "library/constants.h"

// VST3SDK defines RELEASE globally, which leaks into all including code
// And conflict with the RELEASE phase of the Adsr enum.
#undef RELEASE

namespace dsp {

/**
 * @brief Too avoid divisions by zero and extensive branching, we limit the
 * attack, decay and release times to some extremely short value and not 0.
 */
constexpr float SHORTEST_ENVELOPE_TIME = 1.0e-5f;


/**
 * @brief A basic, linear slope, ADSR envelope class.
 */
class AdsrEnvelope
{
    SUSHI_DECLARE_NON_COPYABLE(AdsrEnvelope);

    enum class EnvelopeState
    {
        OFF,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE,
    };

public:
    AdsrEnvelope()
    {};

    /**
     * @brief Set the envelope parameters.
     * @param attack Attack time in seconds.
     * @param decay Decay time in seconds.
     * @param sustain Sustain level, 0 - 1.
     * @param release Release time in seconds.
     */
    void set_parameters(float attack, float decay, float sustain, float release)
    {
        if (attack < SHORTEST_ENVELOPE_TIME) attack = SHORTEST_ENVELOPE_TIME;
        if (decay < SHORTEST_ENVELOPE_TIME) decay = SHORTEST_ENVELOPE_TIME;
        if (release < SHORTEST_ENVELOPE_TIME) release = SHORTEST_ENVELOPE_TIME;

        _attack_factor = 1 / (_samplerate * attack);
        _decay_factor = (1.0f - sustain) / (_samplerate * decay);
        _sustain_level = sustain;
        _release_factor = sustain / (_samplerate * release);
    }

    /**
     * @brief Set the current samplerate
     * @param samplerate The samplerate in samples/second.
     */
    void set_samplerate(float samplerate)
    { _samplerate = samplerate; }

    /**
     * @brief Advance the envelope a given number of samples and return
     *        its current value.
     * @param samples The number of samples to 'tick' the envelope.
     * @return The current envelope level.
     */
    float tick(int samples = 0)
    {
        switch (_state)
        {
            case EnvelopeState::OFF:
                break;

            case EnvelopeState::ATTACK:
                _current_level += samples * _attack_factor;
                if (_current_level >= 1)
                {
                    _state = EnvelopeState::DECAY;
                    _current_level = 1.0f;
                }
                break;

            case EnvelopeState::DECAY:
                _current_level -= samples * _decay_factor;
                if (_current_level <= _sustain_level)
                {
                    _state = EnvelopeState::SUSTAIN;
                    _current_level = _sustain_level;
                }
                break;

            case EnvelopeState::SUSTAIN:
                /* fixed level, wait for a gate release/note off */
                break;

            case EnvelopeState::RELEASE:
                _current_level -= samples * _release_factor;
                if (_current_level < 0.0f)
                {
                    _state = EnvelopeState::OFF;
                    _current_level = 0.0f;
                }
                break;
        }
        return _current_level;
    }

    /**
     * @brief Get the envelopes current level without advancing it.
     * @return The current envelope level.
     */
    float level() const
    { return _current_level; }

    /**
     * @brief Analogous to the gate signal on an analog envelope. Setting gate
     *        to true will start the envelope in the attack phase and setting
     *        it to false will start the release phase.
     * @param gate Set to true for note on and false at note off.
     */
    void gate(bool gate)
    {
        if (gate) /* If the envelope is running, it's simply restarted here */
        {
            _state = EnvelopeState::ATTACK;
            _current_level = 0.0f;
        } else /* Gate off - go to release phase */
        {
            /* if the envelope enters the release phase from the attack or decay
             * phase, the release factor must be rescaled to give the correct slope */
            if (_state != EnvelopeState::SUSTAIN && _sustain_level > 0)
            {
                _release_factor *= (_current_level / _sustain_level);
            }
            _state = EnvelopeState::RELEASE;
        }
    }

    /**
     * @brief Returns true if the envelope is off, ie. the release phase is finished.
     * @return
     */
    bool finished()
    { return _state == EnvelopeState::OFF; }

    /**
     * @brief Resets the envelope to 0 immediately. Bypassing any long release
     *        phase.
     */
    void reset()
    {
        _state = EnvelopeState::OFF;
        _current_level = 0.0f;
    }

private:
    float _attack_factor{0};
    float _decay_factor{0};
    float _sustain_level{1};
    float _release_factor{0.1};
    float _current_level{0};
    float _samplerate{44100};

    EnvelopeState _state{EnvelopeState::OFF};
};

} // end namespace dsp

#endif //SUSHI_ENVELOPES_H
