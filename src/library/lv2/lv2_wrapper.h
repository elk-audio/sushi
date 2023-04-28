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
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_LV2_PLUGIN_H
#define SUSHI_LV2_PLUGIN_H

#include <map>

#include "engine/base_event_dispatcher.h"
#include "library/processor.h"
#include "library/rt_event_fifo.h"
#include "library/midi_encoder.h"
#include "library/midi_decoder.h"
#include "library/constants.h"

#include "lv2_model.h"

namespace sushi {
namespace lv2 {

/* Should match the maximum reasonable number of channels of a plugin */
constexpr int LV2_WRAPPER_MAX_N_CHANNELS = MAX_TRACK_CHANNELS;

constexpr float CONTROL_OUTPUT_REFRESH_RATE = 30.0f;

/**
 * @brief Wrapper around the global LilvWorld instance so that it can be used
 *        with standard c++ smart pointers
 */
class LilvWorldWrapper
{
public:
    ~LilvWorldWrapper();

    bool create_world();

    LilvWorld* world();

private:
    LilvWorld* _world{nullptr};
};

/**
 * @brief internal wrapper class for loading VST plugins and make them accessible as Processor to the Engine.
 */

class LV2_Wrapper : public Processor
{
public:
    SUSHI_DECLARE_NON_COPYABLE(LV2_Wrapper);
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */
    LV2_Wrapper(HostControl host_control, const std::string& lv2_plugin_uri, std::shared_ptr<LilvWorldWrapper> world);

    ~LV2_Wrapper() override = default;

    ProcessorReturnCode init(float sample_rate) override;

    // LV2 does not support changing the sample rate after initialization.
    void configure(float /*sample_rate*/) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    bool bypassed() const override;

    const ParameterDescriptor* parameter_from_id(ObjectId id) const override;

    std::pair<ProcessorReturnCode, float> parameter_value(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, float> parameter_value_in_domain(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, std::string> parameter_value_formatted(ObjectId parameter_id) const override;

    bool supports_programs() const override;

    int program_count() const override;

    int current_program() const override;

    std::string current_program_name() const override;

    std::pair<ProcessorReturnCode, std::string> program_name(int program) const override;

    std::pair<ProcessorReturnCode, std::vector<std::string>> all_program_names() const override;

    ProcessorReturnCode set_program(int program) override;

    ProcessorReturnCode set_state(ProcessorState* state, bool realtime_running) override;

    ProcessorState save_state() const override;

    PluginInfo info() const override;

    static int worker_callback(void* data, EventId id)
    {
        reinterpret_cast<LV2_Wrapper*>(data)->_worker_callback(id);
        return 1;
    }

    static int restore_state_callback(void* data, EventId id)
    {
        reinterpret_cast<LV2_Wrapper *>(data)->_restore_state_callback(id);
        return 1;
    }

    void request_worker_callback(AsyncWorkCallback callback);

private:
    const LilvPlugin* _plugin_handle_from_URI(const std::string& plugin_URI_string);

    void _worker_callback(EventId);

    void _restore_state_callback(EventId);

    void _update_transport();
    uint8_t pos_buf[256];
    LV2_Atom* _lv2_pos{nullptr};
    bool _xport_changed{false};

    /**
     * @brief Iterate over LV2 parameters and register internal FloatParameterDescriptor
     *        for each one of them.
     * @return True if all parameters were registered properly.
     */
    bool _register_parameters();

    void _fetch_plugin_name_and_label();

    void _map_audio_buffers(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer);

    void _deliver_inputs_to_plugin();
    void _deliver_outputs_from_plugin(bool send_ui_updates);

    bool _calculate_control_output_trigger();

    static MidiDataByte _convert_event_to_midi_buffer(RtEvent& event);
    void _flush_event_queue();
    void _process_midi_input(Port* port);
    void _process_midi_output(Port* port);

    void _set_binary_state(ProcessorState* state);

    float* _process_inputs[LV2_WRAPPER_MAX_N_CHANNELS]{};
    float* _process_outputs[LV2_WRAPPER_MAX_N_CHANNELS]{};

    ChunkSampleBuffer _dummy_input{1};

    ChunkSampleBuffer _dummy_output{1};

    std::string _plugin_path;

    std::shared_ptr<LilvWorldWrapper> _world;

    BypassManager _bypass_manager{_bypassed};

    // This queue holds incoming midi events.
    // They are parsed and converted to lv2_evbuf content for LV2 in
    // process_audio(...).
    RtSafeRtEventFifo _incoming_event_queue;

    std::unique_ptr<Model> _model {nullptr};

    // These are not used for other than the Unit tests,
    // to simulate how the wrapper behaves if multithreaded.
    PlayState _previous_play_state {PlayState::PAUSED};
    void _pause_audio_processing();
    void _resume_audio_processing();

    // TODO: These are duplicated in ParameterPreProcessor, used for internal plugins.
    // Eventually LV2 can instead use the same parameter processing subsystem:
    // It has a field units:unit for instantiating an appropriate PreProcessor.
    static float _to_domain(float value_normalized, float min_domain, float max_domain)
    {
        return max_domain + (min_domain - max_domain) / (_min_normalized - _max_normalized) * (value_normalized - _max_normalized);
    }

    static float _to_normalized(float value, float min_domain, float max_domain)
    {
        return _max_normalized + (_min_normalized - _max_normalized) / (min_domain - max_domain) * (value - max_domain);
    }

    static constexpr float _min_normalized{0.0f};
    static constexpr float _max_normalized{1.0f};

    std::map<ObjectId, const ParameterDescriptor*> _parameters_by_lv2_id;

    int _control_output_refresh_interval{0};
    int _control_output_sample_count{0};
};

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_LV2_PLUGIN_H
