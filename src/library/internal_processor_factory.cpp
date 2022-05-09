/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

#include "internal_processor_factory.h"

#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/lfo_plugin.h"
#include "plugins/equalizer_plugin.h"
#include "plugins/arpeggiator_plugin.h"
#include "plugins/sample_player_plugin.h"
#include "plugins/peak_meter_plugin.h"
#include "plugins/transposer_plugin.h"
#include "plugins/step_sequencer_plugin.h"
#include "plugins/cv_to_control_plugin.h"
#include "plugins/control_to_cv_plugin.h"
#include "plugins/wav_writer_plugin.h"
#include "plugins/mono_summing_plugin.h"
#include "plugins/send_return_factory.h"
#include "plugins/sample_delay_plugin.h"
#include "plugins/stereo_mixer_plugin.h"

namespace sushi {

/**
 * @brief Simplified factory only used internally in this file
 */
class BaseInternalPlugFactory
{
public:
    virtual ~BaseInternalPlugFactory() = default;
    virtual std::string_view uid() const = 0;
    virtual std::shared_ptr<Processor> create(const HostControl &host_control) = 0;
};

template<class T>
class InternalFactory : public BaseInternalPlugFactory
{
public:
    std::string_view uid() const override
    {
        return T::static_uid();
    }

    std::shared_ptr<Processor> create(const HostControl &host_control) override
    {
        return std::make_shared<T>(host_control);
    }
};

InternalProcessorFactory::InternalProcessorFactory() : _send_return_factory(std::make_unique<SendReturnFactory>())
{
    /* When adding new internal plugins, make sure they implement static_uid()
     * then add a line here to add them to the factory  */
    _add(std::make_unique<InternalFactory<passthrough_plugin::PassthroughPlugin>>());
    _add(std::make_unique<InternalFactory<gain_plugin::GainPlugin>>());
    _add(std::make_unique<InternalFactory<lfo_plugin::LfoPlugin>>());
    _add(std::make_unique<InternalFactory<equalizer_plugin::EqualizerPlugin>>());
    _add(std::make_unique<InternalFactory<sample_player_plugin::SamplePlayerPlugin>>());
    _add(std::make_unique<InternalFactory<arpeggiator_plugin::ArpeggiatorPlugin>>());
    _add(std::make_unique<InternalFactory<peak_meter_plugin::PeakMeterPlugin>>());
    _add(std::make_unique<InternalFactory<transposer_plugin::TransposerPlugin>>());
    _add(std::make_unique<InternalFactory<step_sequencer_plugin::StepSequencerPlugin>>());
    _add(std::make_unique<InternalFactory<cv_to_control_plugin::CvToControlPlugin>>());
    _add(std::make_unique<InternalFactory<control_to_cv_plugin::ControlToCvPlugin>>());
    _add(std::make_unique<InternalFactory<wav_writer_plugin::WavWriterPlugin>>());
    _add(std::make_unique<InternalFactory<mono_summing_plugin::MonoSummingPlugin>>());
    _add(std::make_unique<InternalFactory<sample_delay_plugin::SampleDelayPlugin>>());
    _add(std::make_unique<InternalFactory<stereo_mixer_plugin::StereoMixerPlugin>>());
}

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>>
InternalProcessorFactory::new_instance(const PluginInfo &plugin_info,
                                       HostControl &host_control,
                                       float sample_rate)
{
    if (plugin_info.uid == send_plugin::SendPlugin::static_uid() ||
        plugin_info.uid == return_plugin::ReturnPlugin::static_uid())
    {
        return _send_return_factory->new_instance(plugin_info, host_control, sample_rate);
    }

    auto processor = _create_internal_plugin(plugin_info.uid, host_control);
    if (processor == nullptr)
    {
        return {ProcessorReturnCode::ERROR, nullptr};
    }
    else
    {
        auto processor_status = processor->init(sample_rate);
        return {processor_status, processor};
    }
}

InternalProcessorFactory::~InternalProcessorFactory() = default;

std::shared_ptr<Processor> sushi::InternalProcessorFactory::_create_internal_plugin(const std::string &uid,
                                                                                    sushi::HostControl &host_control)
{
    auto factory = _internal_plugin_factories.find(uid);
    if (factory == _internal_plugin_factories.end())
    {
        return nullptr;
    }
    return factory->second->create(host_control);
}

void InternalProcessorFactory::_add(std::unique_ptr<BaseInternalPlugFactory> factory)
{
    _internal_plugin_factories.insert({factory->uid(), std::move(factory)});
}

} // namespace sushi
