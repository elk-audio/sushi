/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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

namespace sushi {

InternalProcessorFactory::InternalProcessorFactory()
{

}

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>>
InternalProcessorFactory::new_instance(const engine::PluginInfo &plugin_info,
                                       HostControl &host_control,
                                       float sample_rate)
{
    auto processor = _create_internal_plugin(plugin_info.uid, host_control);
    if (processor == nullptr)
    {
        return {ProcessorReturnCode::ERROR, nullptr};
    }
    else
    {
        processor->init(sample_rate);
        return {ProcessorReturnCode::OK, processor};
    }
}

std::shared_ptr<Processor>
sushi::InternalProcessorFactory::_create_internal_plugin(const std::string &uid, sushi::HostControl &host_control)
{
    if (uid == "sushi.testing.passthrough")
    {
        return std::make_shared<passthrough_plugin::PassthroughPlugin>(host_control);
    }
    else if (uid == "sushi.testing.gain")
    {
        return std::make_shared<gain_plugin::GainPlugin>(host_control);
    }
    else if (uid == "sushi.testing.lfo")
    {
        return std::make_shared<lfo_plugin::LfoPlugin>(host_control);
    }
    else if (uid == "sushi.testing.equalizer")
    {
        return std::make_shared<equalizer_plugin::EqualizerPlugin>(host_control);
    }
    else if (uid == "sushi.testing.sampleplayer")
    {
        return std::make_shared<sample_player_plugin::SamplePlayerPlugin>(host_control);
    }
    else if (uid == "sushi.testing.arpeggiator")
    {
        return std::make_shared<arpeggiator_plugin::ArpeggiatorPlugin>(host_control);
    }
    else if (uid == "sushi.testing.peakmeter")
    {
        return std::make_shared<peak_meter_plugin::PeakMeterPlugin>(host_control);
    }
    else if (uid == "sushi.testing.transposer")
    {
        return std::make_shared<transposer_plugin::TransposerPlugin>(host_control);
    }
    else if (uid == "sushi.testing.step_sequencer")
    {
        return std::make_shared<step_sequencer_plugin::StepSequencerPlugin>(host_control);
    }
    else if (uid == "sushi.testing.cv_to_control")
    {
        return std::make_shared<cv_to_control_plugin::CvToControlPlugin>(host_control);
    }
    else if (uid == "sushi.testing.control_to_cv")
    {
        return std::make_shared<control_to_cv_plugin::ControlToCvPlugin>(host_control);
    }
    else if (uid == "sushi.testing.wav_writer")
    {
        return std::make_shared<wav_writer_plugin::WavWriterPlugin>(host_control);
    }
    else if (uid == "sushi.testing.mono_summing")
    {
        return std::make_shared<mono_summing_plugin::MonoSummingPlugin>(host_control);
    }
    return nullptr;
}

} // namespace sushi