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
 * @brief Real time audio processing engine interface
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_BASE_ENGINE_H
#define SUSHI_BASE_ENGINE_H

#include <memory>
#include <map>
#include <vector>
#include <utility>
#include <bitset>
#include <limits>

#include "library/constants.h"
#include "base_event_dispatcher.h"
#include "engine/track.h"
#include "library/base_performance_timer.h"
#include "library/time.h"
#include "library/sample_buffer.h"
#include "library/types.h"
#include "control_interface.h"

namespace sushi {
namespace engine {

using BitSet32 = std::bitset<std::numeric_limits<uint32_t>::digits>;

struct ControlBuffer
{
    ControlBuffer() : cv_values{0}, gate_values{0} {}

    std::array<float, MAX_ENGINE_CV_IO_PORTS> cv_values;
    BitSet32 gate_values;
};

enum class EngineReturnStatus
{
    OK,
    ERROR,
    INVALID_N_CHANNELS,
    INVALID_PLUGIN_UID,
    INVALID_PLUGIN_NAME,
    INVALID_PLUGIN_TYPE,
    INVALID_PROCESSOR,
    INVALID_PARAMETER,
    INVALID_TRACK,
    INVALID_BUS,
    INVALID_CHANNEL,
    QUEUE_FULL
};

enum class PluginType
{
    INTERNAL,
    VST2X,
    VST3X
};

enum class RealtimeState
{
    STARTING,
    RUNNING,
    STOPPING,
    STOPPED
};

constexpr int ENGINE_TIMING_ID = -1;

class BaseEngine
{
public:
    BaseEngine(float sample_rate) : _sample_rate(sample_rate)
    {}

    virtual ~BaseEngine()
    {}

    float sample_rate()
    {
        return _sample_rate;
    }

    virtual void set_sample_rate(float sample_rate)
    {
        _sample_rate = sample_rate;
    }

    virtual void set_audio_input_channels(int channels)
    {
        _audio_inputs = channels;
    }

    virtual void set_audio_output_channels(int channels)
    {
        _audio_outputs = channels;
    }

    virtual EngineReturnStatus set_cv_input_channels(int channels)
    {
        _cv_inputs = channels;
        return EngineReturnStatus::OK;

    }

    virtual EngineReturnStatus set_cv_output_channels(int channels)
    {
        _cv_outputs = channels;
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_input_channel(int /*engine_channel*/,
                                                           int /*track_channel*/,
                                                           const std::string& /*track_name*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_output_channel(int /*engine_channel*/,
                                                            int /*track_channel*/,
                                                            const std::string& /*track_name*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_input_bus(int /*input_bus */,
                                                       int /*track_bus*/,
                                                       const std::string & /*track_name*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_output_bus(int /*output_bus*/,
                                                        int /*track_bus*/,
                                                        const std::string & /*track_name*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_cv_to_parameter(const std::string& /*processor_name*/,
                                                       const std::string& /*parameter_name*/,
                                                       int /*cv_input_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_cv_from_parameter(const std::string& /*processor_name*/,
                                                         const std::string& /*parameter_name*/,
                                                         int /*cv_output_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_gate_to_processor(const std::string& /*processor_name*/,
                                                         int /*gate_input_id*/,
                                                         int /*note_no*/,
                                                         int /*channel*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_gate_from_processor(const std::string& /*processor_name*/,
                                                           int /*gate_output_id*/,
                                                           int /*note_no*/,
                                                           int /*channel*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_gate_to_sync(int /*gate_input_id*/,
                                                    int /*ppq_ticks*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_sync_to_gate(int /*gate_output_id*/,
                                                    int /*ppq_ticks*/)
    {
        return EngineReturnStatus::OK;
    }


    virtual int n_channels_in_track(int /*track_no*/)
    {
        return 2;
    }

    virtual bool realtime()
    {
        return true;
    }

    virtual void enable_realtime(bool /*enabled*/) {}

    virtual void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer,
                               SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer,
                               ControlBuffer* in_controls,
                               ControlBuffer* out_controls) = 0;

    virtual void update_time(Time /*timestamp*/, int64_t /*samples*/) = 0;

    virtual void set_output_latency(Time /*latency*/) = 0;

    virtual float tempo() = 0;

    virtual void set_tempo(float /*tempo*/) = 0;

    virtual TimeSignature time_signature() = 0;

    virtual void set_time_signature(TimeSignature /*signature*/) = 0;

    virtual PlayingMode transport_mode() = 0;

    virtual void set_transport_mode(PlayingMode /*mode*/) = 0;

    virtual SyncMode tempo_sync_mode() = 0;

    virtual void set_tempo_sync_mode(SyncMode /*mode*/) = 0;

    virtual EngineReturnStatus send_rt_event(RtEvent& event) = 0;

    virtual EngineReturnStatus send_async_event(RtEvent& event) = 0;

    virtual std::pair<EngineReturnStatus, ObjectId> processor_id_from_name(const std::string& /*name*/)
    {
        return std::make_pair(EngineReturnStatus::OK, 0);
    };

    virtual std::pair<EngineReturnStatus, ObjectId> parameter_id_from_name(const std::string& /*processor_name*/,
                                                                           const std::string& /*parameter_name*/)
    {
        return std::make_pair(EngineReturnStatus::OK, 0);
    };

    virtual std::pair<EngineReturnStatus, const std::string> processor_name_from_id(const ObjectId /*id*/)
    {
        return std::make_pair(EngineReturnStatus::OK, "");
    };

    virtual std::pair<EngineReturnStatus, const std::string> parameter_name_from_id(const std::string& /*processor_name*/,
                                                                                    const ObjectId /*id*/)
    {
        return std::make_pair(EngineReturnStatus::OK, "");
    };

    virtual EngineReturnStatus create_track(const std::string & /*track_id*/, int /*channel_count*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus create_multibus_track(const std::string & /*track_id*/, int /*input_busses*/, int /*output_busses*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus delete_track(const std::string & /*track_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus add_plugin_to_track(const std::string & /*track_id*/,
                                                   const std::string & /*uid*/,
                                                   const std::string & /*name*/,
                                                   const std::string & /*file*/,
                                                   PluginType /*plugin_type*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus remove_plugin_from_track(const std::string & /*track_id*/,
                                                        const std::string & /*plugin_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual const Processor* processor(ObjectId /*processor_id*/) const {return nullptr;}

    virtual Processor* mutable_processor(ObjectId /*processor_id*/) {return nullptr;}

    virtual const std::map<std::string, std::unique_ptr<Processor>>& all_processors()
    {
        static std::map<std::string, std::unique_ptr<Processor>> tmp;
        return tmp;
    }

    virtual const std::vector<Track*>& all_tracks()
    {
        static std::vector<Track*> tmp;
        return tmp;
    }

    virtual dispatcher::BaseEventDispatcher* event_dispatcher()
    {
        return nullptr;
    }

    virtual ext::SushiControl* controller()
    {
        return nullptr;
    }

    virtual engine::Transport* transport()
    {
        return nullptr;
    }

    virtual performance::BasePerformanceTimer* performance_timer()
    {
        return nullptr;
    }

    virtual void enable_input_clip_detection(bool /*enabled*/) {}

    virtual void enable_output_clip_detection(bool /*enabled*/) {}

    virtual void print_timings_to_log() {}

protected:
    float _sample_rate;
    int _audio_inputs{0};
    int _audio_outputs{0};
    int _cv_inputs{0};
    int _cv_outputs{0};
};

} // namespace engine
} // namespace sushi

#endif //SUSHI_BASE_ENGINE_H
