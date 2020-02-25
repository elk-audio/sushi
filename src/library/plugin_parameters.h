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
 * @brief Container classes for plugin parameters
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PLUGIN_PARAMETERS_H
#define SUSHI_PLUGIN_PARAMETERS_H

#include <memory>
#include <cmath>
#include <string>
#include <cassert>

#include "library/constants.h"
#include "library/id_generator.h"
#include "library/types.h"

namespace sushi {

enum class ParameterType
{
    FLOAT,
    INT,
    BOOL,
    STRING,
    DATA,
};

/**
 * @brief Base class for describing plugin parameters
 */
class ParameterDescriptor
{
public:
    ParameterDescriptor(const std::string& name,
                        const std::string& label,
                        const std::string& unit,
                        ParameterType type) : _label(label), _name(name), _unit(unit), _type(type) {}

    virtual ~ParameterDescriptor() = default;

    /**
     * @brief Returns the enumerated type of the parameter
     */
    ParameterType type() const {return _type;};

    /**
     * @brief Returns the display name of the parameter, i.e. "Oscillator pitch"
     */
    const std::string& label() const {return _label;}

    /**
    * @brief Returns a unique identifier to the parameter i.e. "oscillator_2_pitch"
    */
    const std::string& name() const {return _name;}

    /**
    * @brief Returns the unit of the parameter i.e. "dB" or "Hz"
    */
    const std::string& unit() const {return _unit;}

    /**
     * @brief Returns a unique identifier for this parameter
     */
    ObjectId id() const {return _id;}

    /**
     * @brief Set a new id
     */
    void set_id(ObjectId id) {_id = id;}

    /**
     * @brief Whether the parameter is automatable or not.
     * @return true if the parameter can be automated, false otherwise
     */
    virtual bool automatable() const {return true;}

    /**
     * @brief Returns the maximum value of the parameter
     * @return A float with the maximum representation of the parameter
     */
    virtual float min_range() const {return 0;};

    /**
     * @brief Returns the minimum value of the parameter
     * @return A float with the minimum representation of the parameter
     */
    virtual float max_range() const {return 1;};

protected:
    std::string _label;
    std::string _name;
    std::string _unit;
    ObjectId _id;
    ParameterType _type;
};


/**
 * @brief Parameter preprocessor for scaling or non-linear mapping. This basic,
 * templated base class only supports clipping to a pre-defined range.
 */
template<typename T>
class ParameterPreProcessor
{
public:
    ParameterPreProcessor(T min, T max): _min_domain_value(min), _max_domain_value(max) {}

    virtual T process(T value)
    {
        return clip(value);
    }

    T lerp_to_domain(float value_normalized)
    {
        return _max_domain_value + (_min_domain_value - _max_domain_value) / (_min_normalized - _max_normalized) * (value_normalized - _max_normalized);
    }

    float lerp_to_normalized(T value)
    {
        return _max_normalized + (_min_normalized - _max_normalized) / (_min_domain_value - _max_domain_value) * (value - _max_domain_value);
    }

protected:
    T clip(T raw_value)
    {
        return (raw_value > _max_domain_value ? _max_domain_value : (raw_value < _min_domain_value ? _min_domain_value : raw_value));
    }

    T _min_domain_value;
    T _max_domain_value;

    static constexpr float _min_normalized{0.0f};
    static constexpr float _max_normalized{1.0f};
};

/**
 * @brief Formatter used to format the parameter value to a string
 */
template<typename T>
class ParameterFormatPolicy
{
protected:
    std::string format(const T value) const {return std::to_string(value);}
};

/*
 * The format() function can then be specialized for types that need special handling.
 */
template <> inline std::string ParameterFormatPolicy<bool>::format(bool value) const
{
    return value? "True": "False";
}

template <> inline std::string ParameterFormatPolicy<std::string*>::format(std::string* value) const
{
    return *value;
}

template <> inline std::string ParameterFormatPolicy<BlobData>::format(BlobData /*value*/) const
{
    /* This parameter type is intended to transfer opaque binary data, and
     * consequently there is no format policy that would work. */
    return "Binary data";
}

/**
 * @brief Templated plugin parameter, works out of the box for native
 * types like float, int, etc. Needs specialization for more complex
 * types, for which the template type should likely be a pointer.
 */
template<typename T, ParameterType enumerated_type>
class TypedParameterDescriptor : public ParameterDescriptor
{
public:
    /**
     * @brief Construct a parameter
     */
    TypedParameterDescriptor(const std::string& name,
                             const std::string& label,
                             const std::string& unit,
                             T range_min,
                             T range_max,
                             ParameterPreProcessor<T>* pre_processor) :
                                        ParameterDescriptor(name, label, unit, enumerated_type),
                                        _pre_processor(pre_processor),
                                        _min(range_min),
                                        _max(range_max) {}

    ~TypedParameterDescriptor() = default;

    // TODO: This is weird, I don't see the point of having a method specifically for casting to float,
    // Nor does the name make sense - it doesn't return a range.
    float min_range() const override {return static_cast<float>(_min);}
    float max_range() const override {return static_cast<float>(_max);}

    T min_value() const {return _min;}
    T max_value() const {return _max;}


private:
    std::unique_ptr<ParameterPreProcessor<T>> _pre_processor;
    T _min;
    T _max;
};

/* Partial specialization for pointer type parameters */
template<typename T, ParameterType enumerated_type>
class TypedParameterDescriptor<T *, enumerated_type> : public ParameterDescriptor
{
public:
    TypedParameterDescriptor(const std::string& name,
                             const std::string& label,
                             const std::string& unit) : ParameterDescriptor(name, label, unit, enumerated_type) {}

    ~TypedParameterDescriptor() = default;

    virtual bool automatable() const override {return false;}
};

/* Partial specialization for pointer type parameters */
//template<typename T, ParameterType enumerated_type>
template<>
class TypedParameterDescriptor<BlobData, ParameterType::DATA> : public ParameterDescriptor
{
public:
    TypedParameterDescriptor(const std::string& name,
                             const std::string& label,
                             const std::string& unit) : ParameterDescriptor(name, label, unit, ParameterType::DATA) {}

    ~TypedParameterDescriptor() {}

    virtual bool automatable() const override {return false;}

};

/*
 * The templated forms are not intended to be accessed directly.
 * Instead, the typedefs below provide direct access to the right
 * type combinations.
 */
typedef ParameterPreProcessor<float> FloatParameterPreProcessor;
typedef ParameterPreProcessor<int>   IntParameterPreProcessor;
typedef ParameterPreProcessor<bool>  BoolParameterPreProcessor;

typedef TypedParameterDescriptor<float, ParameterType::FLOAT>         FloatParameterDescriptor;
typedef TypedParameterDescriptor<int, ParameterType::INT>             IntParameterDescriptor;
typedef TypedParameterDescriptor<bool, ParameterType::BOOL>           BoolParameterDescriptor;
typedef TypedParameterDescriptor<std::string*, ParameterType::STRING> StringPropertyDescriptor;
typedef TypedParameterDescriptor<BlobData, ParameterType::DATA>       DataPropertyDescriptor;


/**
 * @brief Preprocessor example to map from decibels to linear gain.
 */
class dBToLinPreProcessor : public FloatParameterPreProcessor
{
public:
    dBToLinPreProcessor(float min, float max): FloatParameterPreProcessor(min, max) {}

    float process(float value) override
    {
        return powf(10.0f, clip(value) / 20.0f);
    }
};

/**
 * @brief Preprocessor example to map from linear gain to decibels.
 */
class LinTodBPreProcessor : public FloatParameterPreProcessor
{
public:
    LinTodBPreProcessor(float min, float max): FloatParameterPreProcessor(min, max) {}

    float process(float value) override
    {
        return 20.0f * log10(clip(value));
    }
};

template<typename T, ParameterType enumerated_type>
class ParameterValue
{
public:
    ParameterValue(ParameterPreProcessor<T>* pre_processor,
                   T value, ParameterDescriptor* descriptor) : _descriptor(descriptor),
                                                               _pre_processor(pre_processor),
                                                               _raw_value_in_domain(value),
                                                               _value_in_domain(pre_processor->process(value)),
                                                               _value_normalized(pre_processor->lerp_to_normalized(_value_in_domain)) {}

    ParameterType type() const {return _type;}

    T value() const {return _value_in_domain;}
    T raw_value() const {return _raw_value_in_domain;}
    float value_normalized() const { return _value_normalized; }

    ParameterDescriptor* descriptor() const {return _descriptor;}

    void set_values(float value_normalized, float raw_value_normalized)
    {
        _value_normalized = value_normalized;
        _value_in_domain = _pre_processor->lerp_to_domain(value_normalized);
        _raw_value_in_domain = _pre_processor->lerp_to_domain(raw_value_normalized);
    }

    void set(float value_normalized)
    {
        _raw_value_in_domain = _pre_processor->lerp_to_domain(value_normalized);
        _value_in_domain = _pre_processor->process(_pre_processor->lerp_to_domain(value_normalized));

        // 'process' has to always execute on the domain value, hence the additional conversion.
        _value_normalized = _pre_processor->lerp_to_normalized(_value_in_domain);
    }

private:
    ParameterType _type{enumerated_type};
    ParameterDescriptor* _descriptor{nullptr};
    ParameterPreProcessor<T>* _pre_processor{nullptr};
    T _raw_value_in_domain;
    T _value_in_domain;
    float _value_normalized;
};

/* Specialization for bool values, lack a pre_processor */
template<>
class ParameterValue<bool, ParameterType::BOOL>
{
public:
    ParameterValue(bool value, ParameterDescriptor* descriptor) : _descriptor(descriptor),
                                                                  _value_in_domain(value) {}

    ParameterType type() const {return _type;}
    bool value() const {return _value_in_domain;}
    bool raw_value() const {return _value_in_domain;}
    ParameterDescriptor* descriptor() const {return _descriptor;}

    void set_values(bool value, bool raw_value) { _value_in_domain = value; _value_in_domain = raw_value;}
    void set(bool value) { _value_in_domain = value;}

private:
    ParameterType _type{ParameterType::BOOL};
    ParameterDescriptor* _descriptor{nullptr};
    bool _value_in_domain;
};

typedef ParameterValue<bool, ParameterType::BOOL> BoolParameterValue;
typedef ParameterValue<int, ParameterType::INT> IntParameterValue;
typedef ParameterValue<float, ParameterType::FLOAT> FloatParameterValue;

class ParameterStorage
{
public:
    BoolParameterValue* bool_parameter_value()
    {
        assert(_bool_value.type() == ParameterType::BOOL);
        return &_bool_value;
    }

    IntParameterValue* int_parameter_value()
    {
        assert(_int_value.type() == ParameterType::INT);
        return &_int_value;
    }

    FloatParameterValue* float_parameter_value()
    {
        assert(_float_value.type() == ParameterType::FLOAT);
        return &_float_value;
    }

    const BoolParameterValue* bool_parameter_value() const
    {
        assert(_bool_value.type() == ParameterType::BOOL);
        return &_bool_value;
    }

    const IntParameterValue* int_parameter_value() const
    {
        assert(_int_value.type() == ParameterType::INT);
        return &_int_value;
    }

    const FloatParameterValue* float_parameter_value() const
    {
        assert(_float_value.type() == ParameterType::FLOAT);
        return &_float_value;
    }

    ParameterType type() const {return _float_value.type();}

    ObjectId id() const {return _bool_value.descriptor()->id();}

    /* Factory functions for construction */
    static ParameterStorage make_bool_parameter_storage(ParameterDescriptor* descriptor,
                                                        bool default_value)
    {
        BoolParameterValue value(default_value, descriptor);
        return ParameterStorage(value);
    }

    static ParameterStorage make_int_parameter_storage(ParameterDescriptor* descriptor,
                                                       float default_value,
                                                       IntParameterPreProcessor* pre_processor)
    {
        IntParameterValue value(pre_processor, default_value, descriptor);
        return ParameterStorage(value);
    }

    static ParameterStorage make_float_parameter_storage(ParameterDescriptor* descriptor,
                                                         float default_value,
                                                         FloatParameterPreProcessor* pre_processor)
    {
        FloatParameterValue value(pre_processor, default_value, descriptor);
        return ParameterStorage(value);
    }

private:
    ParameterStorage(BoolParameterValue value) : _bool_value(value) {}
    ParameterStorage(IntParameterValue value) : _int_value(value) {}
    ParameterStorage(FloatParameterValue value) : _float_value(value) {}

    union
    {
        BoolParameterValue  _bool_value;
        IntParameterValue   _int_value;
        FloatParameterValue _float_value;
    };
};

/* We need this to be able to copy the ParameterValues by value into a container */
static_assert(std::is_trivially_copyable<ParameterStorage>::value, "");

}  // namespace sushi

#endif //SUSHI_PLUGIN_PARAMETERS_H
