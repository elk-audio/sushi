
#ifndef SUSHI_LIBRARY_PLUGIN_ACCESSORS_H
#define SUSHI_LIBRARY_PLUGIN_ACCESSORS_H

#include "plugins/equalizer_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/wav_writer_plugin.h"
#include "plugins/stereo_mixer_plugin.h"

namespace sushi::internal::gain_plugin
{

class Accessor
{
public:
    explicit Accessor(GainPlugin& plugin) : _plugin(plugin) {}

    [[nodiscard]] FloatParameterValue* gain_parameter()
    {
        return _plugin._gain_parameter;
    }

private:
    GainPlugin& _plugin;
};

}

namespace sushi::internal::equalizer_plugin
{

class Accessor
{
public:
    explicit Accessor(const EqualizerPlugin* plugin) : _const_plugin(plugin) {}

    explicit Accessor(EqualizerPlugin* plugin) : _plugin(plugin) {}

    [[nodiscard]] FloatParameterValue* frequency()
    {
        return _plugin->_frequency;
    }

    [[nodiscard]] FloatParameterValue* gain()
    {
        return _plugin->_gain;
    }

    [[nodiscard]] FloatParameterValue* q()
    {
        return _plugin->_q;
    }

    [[nodiscard]] float const_sample_rate() const
    {
        return _const_plugin->_sample_rate;
    }

private:
    EqualizerPlugin* _plugin {nullptr};
    const EqualizerPlugin* _const_plugin {nullptr};
};

}

namespace sushi::internal::stereo_mixer_plugin
{

class Accessor
{
public:
    explicit Accessor(StereoMixerPlugin& plugin) : _plugin(plugin) {}

    [[nodiscard]] ValueSmootherFilter<float>& ch1_left_gain_smoother()
    {
        return _plugin._ch1_left_gain_smoother;
    }

    [[nodiscard]] ValueSmootherFilter<float>& ch1_right_gain_smoother()
    {
        return _plugin._ch1_right_gain_smoother;
    }

    [[nodiscard]] ValueSmootherFilter<float>& ch2_left_gain_smoother()
    {
        return _plugin._ch2_left_gain_smoother;
    }

    [[nodiscard]] ValueSmootherFilter<float>& ch2_right_gain_smoother()
    {
        return _plugin._ch2_right_gain_smoother;
    }

    [[nodiscard]] FloatParameterValue* ch1_pan()
    {
        return _plugin._ch1_pan;
    }

    [[nodiscard]] FloatParameterValue* ch1_gain()
    {
        return _plugin._ch1_gain;
    }

    [[nodiscard]] FloatParameterValue* ch1_invert_phase()
    {
        return _plugin._ch1_invert_phase;
    }

    [[nodiscard]] FloatParameterValue* ch2_pan()
    {
        return _plugin._ch2_pan;
    }

    [[nodiscard]] FloatParameterValue* ch2_gain()
    {
        return _plugin._ch2_gain;
    }

    [[nodiscard]] FloatParameterValue* ch2_invert_phase()
    {
        return _plugin._ch2_invert_phase;
    }

private:
    StereoMixerPlugin& _plugin;
};

}

namespace sushi::internal::wav_writer_plugin
{

class Accessor
{
public:
    explicit Accessor(WavWriterPlugin& plugin) : _plugin(plugin) {}

    [[nodiscard]] BoolParameterValue* recording_parameter()
    {
        return _plugin._recording_parameter;
    }

    WavWriterStatus start_recording()
    {
        return _plugin._start_recording();
    }

    WavWriterStatus stop_recording()
    {
        return _plugin._stop_recording();
    }

    int write_to_file()
    {
        return _plugin._write_to_file();
    }

private:
    WavWriterPlugin& _plugin;
};

}

#endif //SUSHI_LIBRARY_PLUGIN_ACCESSORS_H
