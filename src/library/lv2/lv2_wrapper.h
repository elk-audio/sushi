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

// Parts of the code in this LV2 folder is adapted from the Jalv LV2 example host:

/**
* Copyright 2007-2016 David Robillard <http://drobilla.net>

* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.

* THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - port.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_LV2_PLUGIN_H
#define SUSHI_LV2_PLUGIN_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <map>

#include "engine/base_event_dispatcher.h"
#include "library/processor.h"
#include "library/rt_event_fifo.h"
#include "library/midi_encoder.h"
#include "library/midi_decoder.h"

#include "lv2_plugin_loader.h"
#include "third-party/lv2/lv2_evbuf.h"

namespace sushi {
namespace lv2 {

/* Should match the maximum reasonable number of channels of a plugin */
constexpr int LV2_WRAPPER_MAX_N_CHANNELS = 8;

/**
 * @brief internal wrapper class for loading VST plugins and make them accessible as Processor to the Engine.
 */

class Lv2Wrapper : public Processor
{
public:
    SUSHI_DECLARE_NON_COPYABLE(Lv2Wrapper)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */
    Lv2Wrapper(HostControl host_control, const std::string& lv2_plugin_uri) :
            Processor(host_control),
            _sample_rate {0},
            _process_inputs {},
            _process_outputs {},
            _double_mono_input {false},
            _plugin_path {lv2_plugin_uri}
    {
        _max_input_channels = LV2_WRAPPER_MAX_N_CHANNELS;
        _max_output_channels = LV2_WRAPPER_MAX_N_CHANNELS;
    }

    virtual ~Lv2Wrapper()
    {
        _cleanup();
    }

    /* Inherited from Processor */
    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    std::pair<ProcessorReturnCode, float> parameter_value(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, float> parameter_value_normalised(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, std::string> parameter_value_formatted(ObjectId parameter_id) const override;

    bool supports_programs() const override;

    int program_count() const override;

    int current_program() const override;

    std::string current_program_name() const override;

    std::pair<ProcessorReturnCode, std::string> program_name(int program) const override;

    std::pair<ProcessorReturnCode, std::vector<std::string>> all_program_names() const override;

    ProcessorReturnCode set_program(int program) override;

    void pause();
    void resume();

private:
    bool _create_ports(const LilvPlugin* plugin);
    std::unique_ptr<Port> _create_port(const LilvPlugin* plugin, int port_index, float default_value);

    /**
     * @brief Tell the plugin that we're done with it and release all resources
     * we allocated during initialization.
     */
    void _cleanup();

    /**
     * @brief Iterate over LV2 parameters and register internal FloatParameterDescriptor
     *        for each one of them.
     * @return True if all parameters were registered properly.
     */
    bool _register_parameters();

    /**
     * @brief For plugins that support stereo I/0 and not mono through SetSpeakerArrangements,
     *        we can provide the plugin with a dual mono input/output instead.
     *        Calling this sets up possible dual mono mode.
     * @param speaker_arr_status True if the plugin supports the given speaker arrangement.
     */
    void _update_mono_mode(bool speaker_arr_status);

    bool _check_for_required_features(const LilvPlugin* plugin);
    void _fetch_plugin_name_and_label();

    void _map_audio_buffers(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer);

    void _deliver_inputs_to_plugin();
    void _deliver_outputs_from_plugin(bool send_ui_updates);

    MidiDataByte _convert_event_to_midi_buffer(RtEvent& event);
    void _flush_event_queue();
    void _process_midi_input(Port* port);
    void _process_midi_output(Port* port);

    void _create_controls(bool writable);

    void _populate_program_list();

    float _sample_rate {0};

    float* _process_inputs[LV2_WRAPPER_MAX_N_CHANNELS];
    float* _process_outputs[LV2_WRAPPER_MAX_N_CHANNELS];

    ChunkSampleBuffer _dummy_input {1};
    ChunkSampleBuffer _dummy_output {1};

    bool _double_mono_input{false};

    std::string _plugin_path;

    // VstTimeInfo _time_info;

    // This queue holds incoming midi events.
    // They are parsed and converted to lv2_evbuf content for LV2 in
    // process_audio(...).
    RtSafeRtEventFifo _incoming_event_queue;

    PluginLoader _loader;
    LV2Model* _model {nullptr};

    PlayState _previous_play_state {PlayState::PAUSED};
};

} // end namespace lv2
} // end namespace sushi


#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

#include "library/processor.h"

namespace sushi {
namespace lv2 {
/* If LV2 support is disabled in the build, the wrapper is replaced with this
   minimal dummy processor whose purpose is to log an error message if a user
   tries to load an LV2 plugin */
class Lv2Wrapper : public Processor
{
public:
    Lv2Wrapper(HostControl host_control, const std::string& /*lv2_plugin_uri*/) :
            Processor(host_control) {}

    ProcessorReturnCode init(float sample_rate) override;
    void process_event(const RtEvent& /*event*/) override {}
    void process_audio(const ChunkSampleBuffer & /*in*/, ChunkSampleBuffer & /*out*/) override {}
};

}// end namespace lv2
}// end namespace sushi
#endif
#endif //SUSHI_LV2_PLUGIN_H