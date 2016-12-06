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
    BaseStompBoxParameter(const std::string& name, StompBoxParameterType type) : _name(name),
                                                                                 _type(type) {}
    virtual ~BaseStompBoxParameter() {}

    /**
     * @brief Returns the enumerated type of the parameter
     */
    StompBoxParameterType type() {return _type;};

    /**
     * @brief Returns the parameters value as a formatted string
     * TODO: Ponder over if we want units included here or as a separate string
     */
    virtual std::string as_string() { return "";} //Need to be implemented, otherwise no vtable will be generated

    /**
     * @brief Returns the name of the parameter, i.e. "Oscillator pitch"
     */
    const std::string name() {return _name;}


protected:
    std::string _name;  // TODO: consider fixed length eastl string here to avoid memory allocations when changing
    StompBoxParameterType _type;
};

/**
 * @brief Parameter preprocessor for scaling or non-linear mapping
 */
template<typename T>
class ParameterPreProcessor
{
public:
    virtual T process(T raw_value) {return raw_value;}
};

/**
 * @brief Preprocessor example to map from decibels to linear gain.
 */
template<typename T>
class dBToLinPreProcessor : public ParameterPreProcessor<T>
{
public:
    T process(T raw_value) override
    {
        return std::pow(10, raw_value / static_cast<T>(20));
    }
};

/**
 * @brief Templated plugin parameter, works out of the box for native
 * types like float, int, etc. Needs specialization for more complex
 * types, for which the template type should likely be a pointer.
 */
template<typename T, StompBoxParameterType enumerated_type>
class StompBoxParameter : public BaseStompBoxParameter
{
public:
    /**
     * @brief Construct a parameter
     */
    StompBoxParameter(const std::string& name,
                      T max_value,
                      T min_value,
                      T default_value,
                      ParameterPreProcessor<T>* preProcessor = nullptr) :
                                   BaseStompBoxParameter(name, enumerated_type),
                                   _preProcessor(preProcessor),
                                   _value(default_value),
                                   _max_range(max_value),
                                   _min_range(min_value) {}
    
    ~StompBoxParameter() {};

    T value()
    {
        return _value.load();
    }

    void set(T value)
    {
        _value.store(_preProcessor->process(value));
    }

    std::string as_string() override
    {
        return std::to_string(_value.load());
    }
private:
    std::unique_ptr<ParameterPreProcessor<T>> _preProcessor;
    std::atomic<T> _value;
    T _max_range;
    T _min_range;
};

typedef dBToLinPreProcessor<float> FloatdBToLinPreProcessor;

typedef StompBoxParameter<float, StompBoxParameterType::FLOAT>  FloatStompBoxParameter;
typedef StompBoxParameter<int, StompBoxParameterType::INT>      IntStompBoxParameter;
typedef StompBoxParameter<bool, StompBoxParameterType::BOOL>    BoolStompBoxParameter;


}  // namespace sushi

#endif //SUSHI_PLUGIN_PARAMETERS_H
