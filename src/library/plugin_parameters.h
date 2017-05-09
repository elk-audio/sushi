/**
 * @Brief Container classes for plugin parameters
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_PLUGIN_PARAMETERS_H
#define SUSHI_PLUGIN_PARAMETERS_H

#include <atomic>
#include <memory>
#include <cmath>
#include <string>

#include "library/constants.h"
#include "library/id_generator.h"

enum class StompBoxParameterType
{
    FLOAT,
    INT,
    BOOL,
    SELECTION,
    STRING,
    DATA,
};

namespace sushi {

/**
 * @brief Base class for plugin parameters
 */
class BaseStompBoxParameter
{
public:
    BaseStompBoxParameter(const std::string& name,
                          const std::string& label,
                          StompBoxParameterType type) : _label(label), _name(name), _type(type) {}

    virtual ~BaseStompBoxParameter() {}

    /**
     * @brief Returns the enumerated type of the parameter
     */
    StompBoxParameterType type() const {return _type;};

    /**
     * @brief Returns the parameters value as a formatted string
     * TODO: Ponder over if we want units included here or as a separate string
     */
    virtual const std::string as_string() { return "";} //Needs to be implemented, otherwise no vtable will be generated

    /**
     * @brief Returns the display name of the parameter, i.e. "Oscillator pitch"
     */
    const std::string& label() const {return _label;}

    /**
    * @brief Returns a unique identifier to the parameter i.e. "oscillator_2_pitch"
    */
    const std::string& name() const {return _name;}

    /**
     * @brief Returns a unique identifier for this parameter
     */
    ObjectId id() const {return _id;}

    /**
     * @brief Set a new id
     */
    void set_id(ObjectId id) {_id = id;}

protected:
    std::string _label;
    std::string _name;
    ObjectId _id;
    StompBoxParameterType _type;
};


/**
 * @brief Parameter preprocessor for scaling or non-linear mapping. This basic,
 * templated base class only supports clipping to a pre-defined range.
 */
template<typename T>
class ParameterPreProcessor
{
public:
    ParameterPreProcessor(T min, T max): _min_range(min), _max_range(max) {}
    virtual T process(T raw_value) {return clip(raw_value);}

protected:
    T clip(T raw_value)
    {
        return (raw_value > _max_range? _max_range : (raw_value < _min_range? _min_range : raw_value));
    }

    T _min_range;
    T _max_range;
};

/**
 * @brief Formatter used to format the parameter value to a string
 */
template<typename T>
class ParameterFormatPolicy
{
protected:
    const std::string format(T value) {return std::to_string(value);}
};

/*
 * The format() function can then be specialized for types that need special handling.
 */
template <> const inline std::string ParameterFormatPolicy<bool>::format(bool value)
{
    return value? "True": "False";
}
template <> const inline std::string ParameterFormatPolicy<std::string*>::format(std::string* value)
{
    return *value;
}
template <> const inline std::string ParameterFormatPolicy<char*>::format(char* /*value*/)
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
template<typename T, StompBoxParameterType enumerated_type>
class StompBoxParameter : public BaseStompBoxParameter, private ParameterFormatPolicy<T>
{
public:
    /**
     * @brief Construct a parameter
     */
    StompBoxParameter(const std::string& name,
                      const std::string& label,
                      T default_value,
                      ParameterPreProcessor<T>* pre_processor) :
                                   BaseStompBoxParameter(name, label, enumerated_type),
                                   _pre_processor(pre_processor),
                                   _raw_value(default_value),
                                   _value(pre_processor->process(default_value)) {}

    ~StompBoxParameter() {};

    /**
     * @brief Returns the parameter's current value.
     */
    T value()
    {
        return _value;
    }

    /**
     * @brief Returns the parameter's unprocessed value.
     */
    T raw_value()
    {
        return _raw_value;
    }

    /**
     * @brief Set the value of the parameter. Called automatically from the host.
     * For changin the value from inside the plugin, call set_asychronously() intead.
     */
    void set(T value)
    {
        _raw_value = value;
        _value = _pre_processor->process(value);
    }

    /**
     * @brief Tell the host to change the value of the parameter. This should be used
     * instead of set() if the plugin itself wants to change the value of a parameter.
     */
    void set_asychronously(T value)
    {
        // TODO - implement!
    }

    /**
     * @brief Returns the parameter's value as a string, i.e. "1.25".
     * TODO - Think about which value we actually want here, raw or processed!
     */
    const std::string as_string() override
    {
        return ParameterFormatPolicy<T>::format(_raw_value);
    }

private:
    std::unique_ptr<ParameterPreProcessor<T>> _pre_processor;
    T _raw_value;
    T _value;
};

/* Partial specialization for pointer type parameters */
template<typename T, StompBoxParameterType enumerated_type>
class StompBoxParameter<T *, enumerated_type> : public BaseStompBoxParameter, private ParameterFormatPolicy<T *>
{
public:
    StompBoxParameter(const std::string& name,
                      const std::string& label,
                      T * default_value) : BaseStompBoxParameter(name, label, enumerated_type),
                                           _value(default_value) {}

    ~StompBoxParameter() {};

    /**
     * @brief Returns the parameter's current value.
     */
    T * value()
    {
        return _value.get();
    }

    void set(T * value)
    {
        /* Note, not atomic and we still need to figure out a data ownership strategy
         * and avoid deleting in the audio thread */
        _value.reset(value);
    }

    /**
     * @brief Tell the host to change the value of the parameter. This should be used
     * instead of set() if the plugin itself wants to change the value of a parameter.
     */
    void set_asychronously(T * value)
    {
        // TODO - implement!
    }

    /**
     * @brief Returns the parameter's value as a string, i.e. "1.25".
     * TODO - Think about which value we actually want here, raw or processed!
     */
    const std::string as_string() override
    {
        return ParameterFormatPolicy<T *>::format(_value.get());
    }

private:
    std::unique_ptr<T> _value;
};



/*
 * The templated forms are not intended to be accessed directly.
 * Instead, the typedefs below provide direct access to the right
 * type combinations.
 */
typedef ParameterPreProcessor<float> FloatParameterPreProcessor;
typedef ParameterPreProcessor<int> IntParameterPreProcessor;
typedef ParameterPreProcessor<bool> BoolParameterPreProcessor;

typedef StompBoxParameter<float, StompBoxParameterType::FLOAT>  FloatStompBoxParameter;
typedef StompBoxParameter<int, StompBoxParameterType::INT>      IntStompBoxParameter;
typedef StompBoxParameter<bool, StompBoxParameterType::BOOL>    BoolStompBoxParameter;
typedef StompBoxParameter<std::string*, StompBoxParameterType::STRING> StringStompBoxParameter;
/* Eventually we might want to wrap the pointer in a struct
 * with size and data pointer instead of a raw char pointer */
typedef StompBoxParameter<char*, StompBoxParameterType::DATA> DataStompBoxParameter;


/**
 * @brief Preprocessor example to map from decibels to linear gain.
 */
class dBToLinPreProcessor : public FloatParameterPreProcessor
{
public:
    dBToLinPreProcessor(float min, float max): FloatParameterPreProcessor(min, max) {}
    float process(float raw_value) override
    {
        return powf(10.0f, this->clip(raw_value) / 20.0f);
    }
};

}  // namespace sushi

#endif //SUSHI_PLUGIN_PARAMETERS_H
