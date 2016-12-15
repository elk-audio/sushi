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
    BaseStompBoxParameter(const std::string& label,
                          const std::string& id,
                          StompBoxParameterType type) : _label(label), _id(id), _type(type) {}

    virtual ~BaseStompBoxParameter() {}

    /**
     * @brief Returns the enumerated type of the parameter
     */
    StompBoxParameterType type() {return _type;};

    /**
     * @brief Returns the parameters value as a formatted string
     * TODO: Ponder over if we want units included here or as a separate string
     */
    virtual const std::string as_string() { return "";} //Needs to be implemented, otherwise no vtable will be generated

    /**
     * @brief Returns the display name of the parameter, i.e. "Oscillator pitch"
     */
    const std::string label() {return _label;}

    /**
    * @brief Returns a unique identifier to the parameter i.e. "oscillator_2_pitch"
    */
    const std::string id() {return _id;}


protected:
    std::string _label;
    std::string _id;  // TODO: consider fixed length eastl string here to avoid memory allocations when changing
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
    ParameterPreProcessor(T max, T min): _max_range(max), _min_range(min) {}
    virtual T process(T raw_value) {return clip(raw_value);}

protected:
    T clip(T raw_value)
    {
        return (raw_value > _max_range? _max_range : (raw_value < _min_range? _min_range : raw_value));
    }
    T _max_range;
    T _min_range;
};

/**
 * @brief Preprocessor example to map from decibels to linear gain.
 */
template<typename T>
class dBToLinPreProcessor : public ParameterPreProcessor<T>
{
public:
    dBToLinPreProcessor(T max, T min): ParameterPreProcessor<T>(max, min) {}
    T process(T raw_value) override
    {
        return std::pow(10, this->clip(raw_value) / static_cast<T>(20));
    }
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
    StompBoxParameter(const std::string& label,
                      const std::string& id,
                      T default_value,
                      ParameterPreProcessor<T>* preProcessor) :
                                   BaseStompBoxParameter(label, id, enumerated_type),
                                   _preProcessor(preProcessor),
                                   _raw_value(default_value),
                                   _value(preProcessor->process(default_value)) {}

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
        _value = _preProcessor->process(value);
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
    std::unique_ptr<ParameterPreProcessor<T>> _preProcessor;
    T _raw_value;
    T _value;
};


/*
 * The templated forms are not intended to be accessed directly.
 * Instead, the typedefs below provide direct access to the right
 * type combinations.
 */
typedef dBToLinPreProcessor<float> FloatdBToLinPreProcessor;

typedef StompBoxParameter<float, StompBoxParameterType::FLOAT>  FloatStompBoxParameter;
typedef StompBoxParameter<int, StompBoxParameterType::INT>      IntStompBoxParameter;
typedef StompBoxParameter<bool, StompBoxParameterType::BOOL>    BoolStompBoxParameter;

}  // namespace sushi

#endif //SUSHI_PLUGIN_PARAMETERS_H
