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
 * @brief Midi i/o plugin example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_TRANSPOSER_PLUGIN_H
#define SUSHI_TRANSPOSER_PLUGIN_H

#include "library/rt_event_fifo.h"
#include "library/internal_plugin.h"


namespace sushi {
namespace transposer_plugin {

class TransposerPlugin : public InternalPlugin, public UidHelper<TransposerPlugin>
{
public:
    TransposerPlugin(HostControl host_control);

    ~TransposerPlugin() = default;

    ProcessorReturnCode init(float sample_rate) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer&/*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/) override;

    static std::string_view static_uid();

private:
    int _transpose_note(int note);

    MidiDataByte _transpose_midi(MidiDataByte midi_msg);

    FloatParameterValue* _transpose_parameter;
};

}// namespace transposer_plugin
}// namespace sushi


#endif //SUSHI_TRANSPOSER_PLUGIN_H
