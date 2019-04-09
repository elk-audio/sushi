/**
 * @Brief Utility class for smoothing parameters or other time dependent values
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_VALUE_SMOOTHER_H
#define SUSHI_VALUE_SMOOTHER_H

#include <chrono>
#include <cmath>
#include <cassert>

namespace sushi {

/**
 * @brief Class that implements smoothing of a value over a set time period using either
 *        linear ramping or filtering through a 1 pole lowpass filter. The time set
 *        with set_lag_time() represents the actual ramping time in the linear ramp
 *        case, and the 90% rise time in the filter case.
 *        The base template is not intended to be used directly but through one of the
 *        aliases ValueSmootherFilter or ValueSmootherRamp.
 * @tparam T The type used for the stored value.
 * @tparam mode The mode of filtering used. Should be either RAMP or FILTER
 */
template <typename T, int mode>
class ValueSmoother
{
public:
    enum Mode : int
    {
        RAMP = 0,
        FILTER
    };
    ValueSmoother() : _current_value(0), _target_value(0) {}

    ValueSmoother(std::chrono::microseconds lag_time, float sample_rate) : _current_value(0),
                                                                           _target_value(0)
    {
        _update_internals(lag_time, sample_rate);
    }

    ValueSmoother(std::chrono::microseconds lag_time, float sample_rate, T init_value) : _current_value(init_value),
                                                                                         _target_value(init_value)
    {
        _update_internals(lag_time, sample_rate);
    }

    ~ValueSmoother() = default;

    /**
     * @brief Set the desired value
     * @param value The new value to set
     */
    void set(T value)
    {
        _target_value = value;
        if constexpr (mode == RAMP)
        {
            _spec.step = (_target_value - _current_value) / _spec.steps;
            _spec.count = _spec.steps;
        }
    };

    /**
     * @brief Set the desired value directly without applying any smoothing
     * @param value The new value to set
     */
    void set_direct(T target_value)
    {
        _target_value = target_value;
        _current_value = target_value;
        if constexpr (mode == RAMP)
        {
            _spec.count = 0;
        }
    }

    /**
     * @brief Read the current value without updating the object
     * @return The current smoothed value
     */
    T value() const  {return _current_value;}

    /**
     * @brief Advance the smoother one sample point and return the new current value
     * @return The current smoothed value
     */
    T next_value()
    {
        if constexpr (mode == Mode::RAMP)
        {
            assert(_spec.steps >= 0);
            if (_spec.count > 0)
            {
                _spec.count--;
                return _current_value += _spec.step;
            }
            return _target_value;
        }
        else
        {
            assert(_spec.coeff != 0);
            return _current_value = (1.0 - _spec.coeff) * _target_value + _spec.coeff * _current_value;
        }
    }

    /**
     * @brief Test whether the smoother has reached the target value.
     * @return true if the value has reached the target value, false otherwise
     */
    bool stationary() const
    {
        if constexpr (mode == Mode::RAMP)
        {
            return _spec.count == 0;
        }
        else
        {
            return std::abs(_target_value - _current_value) < STATIONARY_LIMIT;
        }
    }

    /**
     * @brief Set the smoothing parameters
     * @param lag_time The approximate time to reach the target value
     * @param sample_rate The rate at which the smoother is updated
     */
    void set_lag_time(std::chrono::microseconds lag_time, float sample_rate)
    {
        _update_internals(lag_time, sample_rate);
    }

private:
    struct LinearSpecific
    {
        T   step{0};
        int count{0};
        int steps{-1};
    };
    struct FilterSpecific
    {
        T   coeff{0};
    };

    static constexpr T TIMECONSTANTS_RISE_TIME = 2.19;
    static constexpr T STATIONARY_LIMIT = 0.001;
    static_assert(mode == Mode::RAMP || mode == Mode::FILTER);

    void _update_internals(std::chrono::microseconds lag_time, float sample_rate)
    {
        if constexpr (mode == Mode::FILTER)
        {
            _spec.coeff = std::exp(-1.0 * TIMECONSTANTS_RISE_TIME / (lag_time.count() * sample_rate / 1'000'000));
        }
        else
        {
            _spec.steps = static_cast<int>(std::round(lag_time.count() * sample_rate / 1'000'000));
        }
    }

    T _current_value;
    T _target_value;
    std::conditional_t<mode == Mode::RAMP, LinearSpecific, FilterSpecific> _spec;
};

/* ValueSmoother should be declared through one of the aliases below */
template <typename FloatType>
using ValueSmootherRamp = ValueSmoother<FloatType, ValueSmoother<FloatType,0>::Mode::RAMP>;

template <typename FloatType>
using ValueSmootherFilter = ValueSmoother<FloatType, ValueSmoother<FloatType,0>::Mode::FILTER>;

}  // namespace sushi

#endif //SUSHI_VALUE_SMOOTHER_H

