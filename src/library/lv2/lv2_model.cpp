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
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - model.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2_model.h"
#include "lv2_features.h"
#include "lv2_worker.h"
#include "lv2_wrapper.h"
#include "lv2_state.h"
#include "logging.h"

namespace {
/** These features have no data */
const LV2_Feature static_features[] = {
        { LV2_STATE__loadDefaultState, nullptr },
        { LV2_BUF_SIZE__powerOf2BlockLength, nullptr },
        { LV2_BUF_SIZE__fixedBlockLength, nullptr },
        { LV2_BUF_SIZE__boundedBlockLength, nullptr } };

} // anonymous namespace


namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

Model::Model(float sample_rate, LV2_Wrapper* wrapper, LilvWorld* world): _sample_rate(sample_rate),
                                                                         _wrapper(wrapper),
                                                                         _world(world)
{
    _nodes = std::make_unique<HostNodes>(_world);

    // This allows loading plug-ins from their URI's, assuming they are installed in the correct paths
    // on the local machine.

    _initialize_map_feature();
    _initialize_unmap_feature();
    _initialize_urid_symap();
    _initialize_log_feature();
    _initialize_make_path_feature();

    _initialize_worker_feature();
    _initialize_safe_restore_feature();
    _initialize_options_feature();
}

Model::~Model()
{
    if (_plugin_instance)
    {
        lilv_instance_deactivate(_plugin_instance);
        lilv_instance_free(_plugin_instance);

        for (unsigned i = 0; i < _controls.size(); ++i)
        {
            auto control = _controls[i];
            lilv_node_free(control.node);
            lilv_node_free(control.symbol);
            lilv_node_free(control.label);

            // This can optionally be null for some plugins.
            if (control.group != nullptr)
            {
                lilv_node_free(control.group);
            }

            lilv_node_free(control.min);
            lilv_node_free(control.max);
            lilv_node_free(control.def);
        }

        _plugin_instance = nullptr;

        _lv2_state->unload_programs();
    }

    // Explicitly setting to nullptr, so that destructor is invoked before world is freed.
    _nodes = nullptr;

    symap_free(_symap);
}

void Model::_initialize_host_feature_list()
{
    // Build feature list for passing to plugins.
    // Warning: LV2 / Lilv require this list to be null-terminated.
    // So remember to check for null when iterating over it!
    std::array<const LV2_Feature*, FEATURE_LIST_SIZE> features({
            &_features.map_feature,
            &_features.unmap_feature,
            &_features.log_feature,
            &_features.sched_feature,
            &_features.make_path_feature,
            &_features.options_feature,
            &static_features[0],
            &static_features[1],
            &static_features[2],
            &static_features[3],
            nullptr
    });

    _feature_list = features;
}

bool Model::_feature_is_supported(const std::string& uri)
{
    if (uri.compare("http://lv2plug.in/ns/lv2core#isLive") == 0)
    {
        return true;
    }

    for (const auto f : _feature_list)
    {
        if (f == nullptr) // The last element is by LV2 required to be null.
            break;

        if (uri.compare(f->URI) == 0)
        {
            return true;
        }
    }

    return false;
}

ProcessorReturnCode Model::load_plugin(const LilvPlugin* plugin_handle, double sample_rate)
{
    _plugin_class = plugin_handle;
    _play_state = PlayState::PAUSED;
    _sample_rate = sample_rate;

    _initialize_host_feature_list();

    if (std::getenv("LV2_PATH") == nullptr)
    {
        SUSHI_LOG_ERROR("The LV2_PATH environment variable is not set on your system "
                        "- this is required for loading LV2 plugins.");
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }

    if (_check_for_required_features(plugin_handle) == false)
    {
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }

    _plugin_instance = lilv_plugin_instantiate(plugin_handle, sample_rate, _feature_list.data());

    if (_plugin_instance == nullptr)
    {
        SUSHI_LOG_ERROR("Failed to load LV2 - Plugin entry point not found.");
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }

    _features.ext_data.data_access = lilv_instance_get_descriptor(_plugin_instance)->extension_data;

    /* Create workers if necessary */
    if (lilv_plugin_has_extension_data(plugin_handle, _nodes->work_interface))
    {
        const void* iface_raw = lilv_instance_get_extension_data(_plugin_instance, LV2_WORKER__interface);
        auto iface = static_cast<const LV2_Worker_Interface*>(iface_raw);

        _worker->init(iface, true);
        if (_safe_restore)
        {
            _state_worker->init(iface, false);
        }
    }

    auto state_threadSafeRestore = lilv_new_uri(_world, LV2_STATE__threadSafeRestore);
    if (lilv_plugin_has_feature(plugin_handle, state_threadSafeRestore))
    {
        _safe_restore = true;
    }
    lilv_node_free(state_threadSafeRestore);

    if (_create_ports(plugin_handle) == false)
    {
        SUSHI_LOG_ERROR("Failed to allocate ports for LV2 plugin.");
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }

    _lv2_state = std::make_unique<State>(this);

    _lv2_state->populate_program_list();

    auto state = lilv_state_new_from_world(_world,
                                           &get_map(),
                                           lilv_plugin_get_uri(plugin_handle));

    if (state != nullptr) // Apply loaded state to plugin instance if necessary
    {
        _lv2_state->apply_state(state, false);
        lilv_state_free(state);
    }

    _create_controls(true);
    _create_controls(false);

    return ProcessorReturnCode::OK;
}

bool Model::_create_ports(const LilvPlugin* plugin)
{
    _input_audio_channel_count = 0;
    _output_audio_channel_count = 0;

    const int port_count = lilv_plugin_get_num_ports(plugin);

    std::vector<float> default_values(port_count);

    lilv_plugin_get_port_ranges_float(plugin, nullptr, nullptr, default_values.data());

    for (int i = 0; i < port_count; ++i)
    {
        auto new_port = _create_port(plugin, i, default_values[i]);

        // If it fails to create a new port, loading has failed, and the host shuts down.
        if(new_port.flow() != PortFlow::FLOW_INPUT &&
           new_port.flow() != PortFlow::FLOW_OUTPUT &&
           new_port.optional() == false)
        {
            SUSHI_LOG_ERROR("Mandatory LV2 port has unknown type (neither input nor output)");
            return false;
        }

        // If the port has an unknown data type, loading has failed, and the host shuts down.
        if( new_port.type() != PortType::TYPE_CONTROL &&
            new_port.type() != PortType::TYPE_AUDIO &&
            new_port.type() != PortType::TYPE_EVENT &&
            new_port.optional() == false)
        {
            SUSHI_LOG_ERROR("Mandatory LV2 port has unknown data type.");
            return false;
        }

        add_port(new_port);
    }

    const auto control_input = lilv_plugin_get_port_by_designation(plugin,
                                                                   nodes()->lv2_InputPort,
                                                                   nodes()->lv2_control);

    // The (optional) lv2:designation of this port is lv2:control,
    // which indicates that this is the "main" control port where the host should send events
    // it expects to configure the plugin, for example changing the MIDI program.
    // This is necessary since it is possible to have several MIDI input ports,
    // though typically it is best to have one.
    if (control_input != nullptr)
    {
        set_control_input_index(lilv_port_get_index(plugin, control_input));
    }

    return true;
}

/**
Create a port from data description. This is called before plugin
and Jack instantiation. The remaining instance-specific setup
(e.g. buffers) is done later in activate_port().

Exception Port::FailedCreation can be thrown in the port constructor!
*/
Port Model::_create_port(const LilvPlugin* plugin, int port_index, float default_value)
{
    Port port(plugin, port_index, default_value, this);

    if (port.type() == PortType::TYPE_AUDIO)
    {
        if (port.flow() == PortFlow::FLOW_INPUT)
        {
            _input_audio_channel_count++;
        }
        else if (port.flow() == PortFlow::FLOW_OUTPUT)
        {
            _output_audio_channel_count++;
        }
    }

    return port;
}

void Model::_create_controls(bool writable)
{
    const auto uri_node = lilv_plugin_get_uri(_plugin_class);
    auto patch_writable = lilv_new_uri(_world, LV2_PATCH__writable);
    auto patch_readable = lilv_new_uri(_world, LV2_PATCH__readable);
    const std::string uri_as_string = lilv_node_as_string(uri_node);

    auto properties = lilv_world_find_nodes(
            _world,
            uri_node,
            writable ? patch_writable : patch_readable,
            nullptr);

    LILV_FOREACH(nodes, p, properties)
    {
        const auto property = lilv_nodes_get(properties, p);

        bool found = false;
        if ((writable == false) && lilv_world_ask(_world,
                                                  uri_node,
                                                  patch_writable,
                                                  property))
        {
            // Find existing writable control
            for (auto& control : _controls)
            {
                if (lilv_node_equals(control.node, property))
                {
                    found = true;
                    control.is_readable = true;
                    break;
                }
            }

            if (found)
            {
                continue; // This skips subsequent.
            }
        }

        auto record = ControlID::new_property_control(this, property);

        if (writable)
        {
            record.is_writable = true;
        }
        else
        {
            record.is_readable = true;
        }

        if (record.value_type)
        {
            _controls.push_back(std::move(record));
        }
        else
        {
            SUSHI_LOG_ERROR("Parameter {} has unknown value type, ignored", lilv_node_as_string(record.node));
        }
    }

    lilv_nodes_free(properties);

    lilv_node_free(patch_readable);
    lilv_node_free(patch_writable);
}

void Model::_initialize_urid_symap()
{
    lv2_atom_forge_init(&this->_forge, &_map);

    this->_urids.atom_Float = symap_map(this->_symap, LV2_ATOM__Float);
    this->_urids.atom_Int = symap_map(this->_symap, LV2_ATOM__Int);
    this->_urids.atom_Object = symap_map(this->_symap, LV2_ATOM__Object);
    this->_urids.atom_Path = symap_map(this->_symap, LV2_ATOM__Path);
    this->_urids.atom_String = symap_map(this->_symap, LV2_ATOM__String);
    this->_urids.atom_eventTransfer = symap_map(this->_symap, LV2_ATOM__eventTransfer);
    this->_urids.bufsz_maxBlockLength = symap_map(this->_symap, LV2_BUF_SIZE__maxBlockLength);
    this->_urids.bufsz_minBlockLength = symap_map(this->_symap, LV2_BUF_SIZE__minBlockLength);
    this->_urids.bufsz_sequenceSize = symap_map(this->_symap, LV2_BUF_SIZE__sequenceSize);
    this->_urids.log_Error = symap_map(this->_symap, LV2_LOG__Error);
    this->_urids.log_Trace = symap_map(this->_symap, LV2_LOG__Trace);
    this->_urids.log_Warning = symap_map(this->_symap, LV2_LOG__Warning);
    this->_urids.log_Entry = symap_map(this->_symap, LV2_LOG__Entry);
    this->_urids.log_Note = symap_map(this->_symap, LV2_LOG__Note);
    this->_urids.log_log = symap_map(this->_symap, LV2_LOG__log);
    this->_urids.midi_MidiEvent = symap_map(this->_symap, LV2_MIDI__MidiEvent);
    this->_urids.param_sampleRate = symap_map(this->_symap, LV2_PARAMETERS__sampleRate);
    this->_urids.patch_Get = symap_map(this->_symap, LV2_PATCH__Get);
    this->_urids.patch_Put = symap_map(this->_symap, LV2_PATCH__Put);
    this->_urids.patch_Set = symap_map(this->_symap, LV2_PATCH__Set);
    this->_urids.patch_body = symap_map(this->_symap, LV2_PATCH__body);
    this->_urids.patch_property = symap_map(this->_symap, LV2_PATCH__property);
    this->_urids.patch_value = symap_map(this->_symap, LV2_PATCH__value);
    this->_urids.time_Position = symap_map(this->_symap, LV2_TIME__Position);
    this->_urids.time_bar = symap_map(this->_symap, LV2_TIME__bar);
    this->_urids.time_barBeat = symap_map(this->_symap, LV2_TIME__barBeat);
    this->_urids.time_beatUnit = symap_map(this->_symap, LV2_TIME__beatUnit);
    this->_urids.time_beatsPerBar = symap_map(this->_symap, LV2_TIME__beatsPerBar);
    this->_urids.time_beatsPerMinute = symap_map(this->_symap, LV2_TIME__beatsPerMinute);
    this->_urids.time_frame = symap_map(this->_symap, LV2_TIME__frame);
    this->_urids.time_speed = symap_map(this->_symap, LV2_TIME__speed);
    this->_urids.ui_updateRate = symap_map(this->_symap, LV2_UI__updateRate);
}

void Model::_initialize_log_feature()
{
    this->_features.llog.handle = this;
    this->_features.llog.printf = lv2_printf;
    this->_features.llog.vprintf = lv2_vprintf;
    init_feature(&_features.log_feature, LV2_LOG__log, &_features.llog);
}

void Model::_initialize_map_feature()
{
    this->_symap = lv2_host::symap_new();
    this->_map.handle = this;
    this->_map.map = map_uri;
    init_feature(&this->_features.map_feature, LV2_URID__map, &this->_map);
}

void Model::_initialize_unmap_feature()
{
    this->_unmap.handle = this;
    this->_unmap.unmap = unmap_uri;
    init_feature(&this->_features.unmap_feature, LV2_URID__unmap, &this->_unmap);
}

State* Model::state()
{
    return _lv2_state.get();
}

void Model::_initialize_make_path_feature()
{
    this->_features.make_path.handle = this;
    this->_features.make_path.path = make_path;
    init_feature(&this->_features.make_path_feature,
                 LV2_STATE__makePath, &this->_features.make_path);
}

void Model::_initialize_worker_feature()
{
    _worker = std::make_unique<Worker>(this);

    _features.sched.handle = _worker.get();
    _features.sched.schedule_work = Worker::schedule;
    init_feature(&_features.sched_feature,
                 LV2_WORKER__schedule, &_features.sched);
}

void Model::_initialize_safe_restore_feature()
{
    _state_worker = std::make_unique<Worker>(this);

    _features.ssched.handle = _state_worker.get();
    _features.ssched.schedule_work = Worker::schedule;
    init_feature(&_features.state_sched_feature,
                 LV2_WORKER__schedule, &_features.ssched);

    init_feature(&this->_features.safe_restore_feature,
                 LV2_STATE__threadSafeRestore,
                 nullptr);
}

void Model::_initialize_options_feature()
{
    /* Build options array to pass to plugin */
    const LV2_Options_Option options[6] = {
            { LV2_OPTIONS_INSTANCE, 0, _urids.param_sampleRate, sizeof(float), _urids.atom_Float, &_sample_rate },
            { LV2_OPTIONS_INSTANCE, 0, _urids.bufsz_minBlockLength, sizeof(int32_t), _urids.atom_Int, &_buffer_size },
            { LV2_OPTIONS_INSTANCE, 0, _urids.bufsz_maxBlockLength, sizeof(int32_t), _urids.atom_Int, &_buffer_size },
            { LV2_OPTIONS_INSTANCE, 0, _urids.bufsz_sequenceSize, sizeof(int32_t), _urids.atom_Int, &_midi_buffer_size },
            { LV2_OPTIONS_INSTANCE, 0, _urids.ui_updateRate, sizeof(float), _urids.atom_Float, &_ui_update_hz },
            { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr }
    };

    std::copy(std::begin(options), std::end(options), std::begin(_features.options));

    init_feature(&_features.options_feature,
                 LV2_OPTIONS__options,
                 (void*)_features.options.data());
}

bool Model::_check_for_required_features(const LilvPlugin* plugin)
{
    // Check that any required features are supported
    auto required_features = lilv_plugin_get_required_features(plugin);

    LILV_FOREACH(nodes, f, required_features)
    {
        auto node = lilv_nodes_get(required_features, f);
        auto uri = lilv_node_as_uri(node);

        if (_feature_is_supported(uri) == false)
        {
            SUSHI_LOG_ERROR("LV2 feature {} is not supported.", uri);

            return false;
        }
    }

    lilv_nodes_free(required_features);
    return true;
}

std::array<const LV2_Feature*, FEATURE_LIST_SIZE>* Model::host_feature_list()
{
    return &_feature_list;
}

LilvWorld* Model::lilv_world()
{
    return _world;
}

LilvInstance* Model::plugin_instance()
{
    return _plugin_instance;
}

const LilvPlugin* Model::plugin_class()
{
    return _plugin_class;
}

int Model::midi_buffer_size() const
{
    return _midi_buffer_size;
}

float Model::sample_rate() const
{
    return _sample_rate;
}

Port* Model::get_port(int index)
{
    return &_ports[index];
}

void Model::add_port(Port port)
{
    _ports.push_back(port);
}

int Model::port_count()
{
    return _ports.size();
}

const HostNodes* Model::nodes()
{
    return _nodes.get();
}

const LV2_URIDs& Model::urids()
{
    return _urids;
}

LV2_URID_Map& Model::get_map()
{
    return _map;
}

LV2_URID_Unmap& Model::get_unmap()
{
    return _unmap;
}

LV2_URID Model::map(const char* uri)
{
    std::unique_lock<std::mutex> lock(_symap_lock);
    return symap_map(_symap, uri);
}

const char* Model::unmap(LV2_URID urid)
{
    std::unique_lock<std::mutex> lock(_symap_lock);
    const char* uri = symap_unmap(_symap, urid);

    return uri;
}

LV2_Atom_Forge& Model::forge()
{
    return _forge;
}

int Model::plugin_latency() const
{
    return _plugin_latency;
}

void Model::set_plugin_latency(int latency)
{
    _plugin_latency = latency;
}

void Model::set_control_input_index(int index)
{
    _control_input_index = index;
}

bool Model::update_requested() const
{
    return _request_update;
}

void Model::request_update()
{
    _request_update = true;
}

void Model::clear_update_request()
{
    _request_update = false;
}

void Model::set_play_state(PlayState play_state)
{
    _play_state = play_state;
}

PlayState Model::play_state() const
{
    return _play_state;
}

std::string Model::temp_dir()
{
    return _temp_dir;
}

std::string Model::save_dir()
{
    return _save_dir;
}

void Model::set_save_dir(const std::string& save_dir)
{
    _save_dir = save_dir;
}

bool Model::buf_size_set() const
{
    return _buf_size_set;
}

std::vector<ControlID>& Model::controls()
{
    return _controls;
}

uint32_t Model::position() const
{
    return _position;
}

void Model::set_position(uint32_t position)
{
    _position = position;
}

float Model::bpm() const
{
    return _bpm;
}

void Model::set_bpm(float bpm)
{
    _bpm = bpm;
}

bool Model::rolling() const
{
    return _rolling;
}

void Model::set_rolling(bool rolling)
{
    _rolling = rolling;
}

std::pair<LilvState*, bool> Model::state_to_set()
{
    return {_state_to_set, _delete_state_after_use};
}

void Model::set_state_to_set(LilvState* state_to_set, bool delete_after_use)
{
    _state_to_set = state_to_set;
    _delete_state_after_use = delete_after_use;
}

int Model::input_audio_channel_count() const
{
    return _input_audio_channel_count;
}

int Model::output_audio_channel_count() const
{
    return _output_audio_channel_count;
}

Worker* Model::worker()
{
    return _worker.get();
}

Worker* Model::state_worker()
{
    return _state_worker.get();
}

bool Model::safe_restore()
{
    return _safe_restore;
}

LV2_Wrapper* Model::wrapper()
{
    return _wrapper;
}

}
}

#endif //SUSHI_BUILD_WITH_LV2
