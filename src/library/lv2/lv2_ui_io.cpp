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
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include <csignal>

#include "lv2_features.h"
#include "library/rt_event_fifo.h"
#include "lv2_ui_io.h"
#include "lv2_control.h"
#include "logging.h"

namespace sushi {
namespace lv2 {

    /* Size factor for UI ring buffers.  The ring size is a few times the size of
   an event output to give the UI a chance to keep up. Experiments with Ingen,
   which can highly saturate its event output, led me to this value. It
   really ought to be enough for anybody(TM).
*/
#define N_BUFFER_CYCLES 16

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

void Lv2_UI_IO::init(const LilvPlugin* plugin, float sample_rate, int midi_buf_size)
{
    /* Get a plugin UI */
    _uis = lilv_plugin_get_uis(plugin);
    _ui = lilv_uis_get(_uis, lilv_uis_begin(_uis));

    /* Create ringbuffers for UI if necessary */
    if (_ui)
    {
        auto ui_uri = lilv_ui_get_uri(_ui);
        auto ui_name = lilv_node_as_uri(ui_uri);
        fprintf(stderr, "UI: %s\n", ui_name);
    }
    else
    {
        fprintf(stderr, "UI: None\n");
    }

    if (_buffer_size == 0)
    {
        /* The UI ring is fed by plugin output ports (usually one), and the UI
           updates roughly once per cycle.  The ring size is a few times the
           size of the MIDI output to give the UI a chance to keep up.  The UI
           should be able to keep up with 4 cycles, and tests show this works
           for me, but this value might need increasing to avoid overflows.
        */
        _buffer_size = midi_buf_size * N_BUFFER_CYCLES;
    }

    if (_update_rate == 0.0)
    {
        /* Calculate a reasonable UI update frequency. */
        ui_update_hz = sample_rate / midi_buf_size * 2.0f;
        ui_update_hz = std::max(25.0f, ui_update_hz);
    }
    else
    {
        /* Use user-specified UI update rate. */
        ui_update_hz = _update_rate;
        ui_update_hz = std::max(1.0f, ui_update_hz);
    }

    /* The UI can only go so fast, clamp to reasonable limits */
    ui_update_hz = std::min(60.0f, ui_update_hz);
    _buffer_size = std::max(4096, static_cast<int>(_buffer_size));
    fprintf(stderr, "Comm buffers: %d bytes\n", _buffer_size);
    fprintf(stderr, "Update rate:  %.01f Hz\n", ui_update_hz);

    /* Create Plugin <=> UI communication buffers */


    _ui_events = zix_ring_new(_buffer_size);
    zix_ring_mlock(_ui_events);

    _plugin_events = zix_ring_new(_buffer_size);
    zix_ring_mlock(_plugin_events);
}

void Lv2_UI_IO::write_ui_event(const char* buf)
{
    if (zix_ring_write(_plugin_events, buf, sizeof(buf)) < sizeof(buf))
    {
        SUSHI_LOG_ERROR("Plugin => UI buffer overflow!");
    }
}

void Lv2_UI_IO::set_buffer_size(uint32_t buffer_size)
{
    _buffer_size = std::max(_buffer_size, buffer_size * N_BUFFER_CYCLES);
}

// TODO: Currently unused
void Lv2_UI_IO::set_control(const ControlID* control, uint32_t size, LV2_URID type, const void* body)
{
    auto model = control->model;

    if (control->type == ControlType::PORT && type == model->get_forge().Float)
    {
        auto port = control->model->get_port(control->index);
        port->control = *static_cast<const float*>(body);
    }
    else if (control->type == ControlType::PROPERTY)
    {
        // Copy forge since it is used by process thread
        LV2_Atom_Forge forge = model->get_forge();
        LV2_Atom_Forge_Frame frame;
        uint8_t buf[1024];
        lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));

        auto urids = model->get_urids();

        lv2_atom_forge_object(&forge, &frame, 0, urids.patch_Set);
        lv2_atom_forge_key(&forge, urids.patch_property);
        lv2_atom_forge_urid(&forge, control->property);
        lv2_atom_forge_key(&forge, urids.patch_value);
        lv2_atom_forge_atom(&forge, size, type);
        lv2_atom_forge_write(&forge, body, size);

        auto atom = lv2_atom_forge_deref(&forge, frame.ref);
        ui_write(model,
                 model->get_control_input_index(),
                 lv2_atom_total_size(atom),
                 urids.atom_eventTransfer,
                 atom);
    }
}

// TODO Currently unused
void Lv2_UI_IO::ui_instantiate(LV2Model* model, const char* native_ui_type, void* parent)
{
#ifdef HAVE_SUIL
        jalv->ui_host = suil_host_new(jalv_ui_write, jalv_ui_port_index, NULL, NULL);

	const LV2_Feature parent_feature = {
		LV2_UI__parent, parent
	};
	const LV2_Feature instance_feature = {
		NS_EXT "instance-access", lilv_instance_get_handle(jalv->instance)
	};
	const LV2_Feature data_feature = {
		LV2_DATA_ACCESS_URI, &jalv->features.ext_data
	};
	const LV2_Feature idle_feature = {
		LV2_UI__idleInterface, NULL
	};
	const LV2_Feature* ui_features[] = {
		&jalv->features.map_feature,
		&jalv->features.unmap_feature,
		&instance_feature,
		&data_feature,
		&jalv->features.log_feature,
		&parent_feature,
		&jalv->features.options_feature,
		&idle_feature,
		NULL
	};

	const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(_ui));
	const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(_ui));
	char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
	char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);

	jalv->ui_instance = suil_instance_new(
		jalv->ui_host,
		jalv,
		native_ui_type,
		lilv_node_as_uri(lilv_plugin_get_uri(jalv->plugin)),
		lilv_node_as_uri(lilv_ui_get_uri(jalv->_ui)),
		lilv_node_as_uri(_ui_type),
		bundle_path,
		binary_path,
		ui_features);

	lilv_free(binary_path);
	lilv_free(bundle_path);
#endif
}

// TODO: Currently unused.
bool Lv2_UI_IO::ui_is_resizable(LV2Model* model)
{
    if (!_ui)
    {
        return false;
    }

    auto s = lilv_ui_get_uri(_ui);

    auto world = model->get_world();

    auto p = lilv_new_uri(world, LV2_CORE__optionalFeature);
    auto fs = lilv_new_uri(world, LV2_UI__fixedSize);
    auto nrs = lilv_new_uri(world, LV2_UI__noUserResize);

    auto fs_matches = lilv_world_find_nodes(world, s, p, fs);
    auto nrs_matches = lilv_world_find_nodes(world, s, p, nrs);

    lilv_nodes_free(nrs_matches);
    lilv_nodes_free(fs_matches);
    lilv_node_free(nrs);
    lilv_node_free(fs);
    lilv_node_free(p);

    return !fs_matches && !nrs_matches;
}

void Lv2_UI_IO::ui_write(void* const jalv_handle, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, const void* buffer)
{
    auto model  = static_cast<LV2Model*>(jalv_handle);

    if (protocol != 0 && protocol != model->get_urids().atom_eventTransfer)
    {
        fprintf(stderr, "UI write with unsupported protocol %d (%s)\n",
                protocol, unmap_uri(model, protocol));
        return;
    }

    if (port_index >= model->get_port_count())
    {
        fprintf(stderr, "UI write to out of range port index %d\n",
                port_index);
        return;
    }

    // We no longer have command line options for module.
    /*model->opts.dump &&*/
    // This is just for Logging.
    // TODO: Port to use MIND logger!
    // TODO: It is the only code depending on sratom which I don't currently include.
    /* if (protocol == model->urids.atom_eventTransfer)
    {
        const LV2_Atom* atom = (const LV2_Atom*)buffer;

        char* str  = sratom_to_turtle(
                model->sratom, &model->unmap, "jalv:", NULL, NULL,
                atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));

        jalv_ansi_start(stdout, 36);
        printf("\n## UI => Plugin (%u bytes) ##\n%s\n", atom->size, str);

        jalv_ansi_reset(stdout);
        free(str);
    }
    */

    char buf[sizeof(ControlChange) + buffer_size];
    ControlChange* ev = (ControlChange*)buf;
    ev->index = port_index;
    ev->protocol = protocol;
    ev->size = buffer_size;
    memcpy(ev->body, buffer, buffer_size);

    zix_ring_write(_ui_events, buf, sizeof(buf));
}

// TODO: Currently unused.
void Lv2_UI_IO::apply_ui_events(LV2Model* model, uint32_t nframes)
{
    if (!model->has_ui)
    {
        return;
    }

    ControlChange ev;
    const size_t space = zix_ring_read_space(_ui_events);

    for (size_t i = 0; i < space; i += sizeof(ev) + ev.size)
    {
        zix_ring_read(_ui_events, (char*)&ev, sizeof(ev));
        char body[ev.size];

        if (zix_ring_read(_ui_events, body, ev.size) != ev.size)
        {
            fprintf(stderr, "error: Error reading from UI ring buffer\n");
            break;
        }

        assert(ev.index < model->get_port_count());
        auto port = model->get_port(ev.index);

        if (ev.protocol == 0)
        {
            assert(ev.size == sizeof(float));
            port->control = *(float*)body;
        }
        else if (ev.protocol == model->get_urids().atom_eventTransfer)
        {
            LV2_Evbuf_Iterator e = lv2_evbuf_end(port->_evbuf);
            auto atom = reinterpret_cast<const LV2_Atom*>(body);
            lv2_evbuf_write(&e, nframes, 0, atom->type, atom->size, reinterpret_cast<const uint8_t*>(LV2_ATOM_BODY_CONST(atom)));
        }
        else
        {
            fprintf(stderr,"error: Unknown control change protocol %d\n",
                    ev.protocol);
        }
    }
}

// TODO: Essentially unused passed as reference to commented code
uint32_t Lv2_UI_IO::ui_port_index(void* const controller, const char* symbol)
{
    auto model = static_cast<LV2Model*>(controller);
    auto port = port_by_symbol(model, symbol);

    return port ? port->getIndex() : LV2UI_INVALID_PORT_INDEX;
}

// TODO: Currently unused.
void Lv2_UI_IO::init_ui(LV2Model* model)
{
    // Set initial control port values
    for (uint32_t i = 0; i < model->get_port_count(); ++i)
    {
        if (model->get_port(i)->getType() == TYPE_CONTROL)
        {
            ui_port_event(model, i, sizeof(float),0, &model->get_port(i)->control);
        }
    }

    if (model->get_control_input_index() != (uint32_t)-1)
    {
        // Send patch:Get message for initial parameters/etc
        auto forge = model->get_forge();
        LV2_Atom_Forge_Frame frame;
        uint8_t buf[1024];
        lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));
        lv2_atom_forge_object(&forge, &frame, 0, model->get_urids().patch_Get);

        auto atom = lv2_atom_forge_deref(&forge, frame.ref);
        ui_write(model,
                 model->get_control_input_index(),
                 lv2_atom_total_size(atom),
                 model->get_urids().atom_eventTransfer,
                 atom);

        lv2_atom_forge_pop(&forge, &frame);
    }
}

void  Lv2_UI_IO::ui_port_event(LV2Model* model, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, const void* buffer)
{
// TODO: Should this be populated?

//    if (model->ui_instance)
//    {
//        suil_instance_port_event(model->ui_instance, port_index,
//                                 buffer_size, protocol, buffer);
//        return;
//    }
//    else
//    if (protocol == 0 && (Controller*)model->ports[port_index].widget) {
//        control_changed(model,
//                        (Controller*)model->ports[port_index].widget,
//                        buffer_size,
//                        model->forge.Float,
//                        buffer);
//        return;
//    }
//    else if (protocol == 0)
//    {
//        return;  // No widget (probably notOnGUI)
//    }
//    else if (protocol != model->urids.atom_eventTransfer)
//    {
//        fprintf(stderr, "Unknown port event protocol\n");
//        return;
//    }
//
//    const LV2_Atom* atom = (const LV2_Atom*)buffer;
//    if (lv2_atom_forge_is_object_type(&model->forge, atom->type))
//    {
//        updating = true;
//        const LV2_Atom_Object* obj = (const LV2_Atom_Object*)buffer;
//        if (obj->body.otype == model->urids.patch_Set) {
//            const LV2_Atom_URID* property = NULL;
//            const LV2_Atom*      value    = NULL;
//            if (!patch_set_get(model, obj, &property, &value)) {
//                property_changed(model, property->body, value);
//            }
//        } else if (obj->body.otype == model->urids.patch_Put) {
//            const LV2_Atom_Object* body = NULL;
//            if (!patch_put_get(model, obj, &body)) {
//                LV2_ATOM_OBJECT_FOREACH(body, prop) {
//                    property_changed(model, prop->key, &prop->value);
//                }
//            }
//        } else {
//            printf("Unknown object type?\n");
//        }
//        updating = false;
//    }
}

bool Lv2_UI_IO::send_to_ui(LV2Model* model, uint32_t port_index, uint32_t type, uint32_t size, const void* body)
{
    /* TODO (inherited): Be more discriminate about what to send */
    char evbuf[sizeof(ControlChange) + sizeof(LV2_Atom)];
    ControlChange* ev = (ControlChange*)evbuf;
    ev->index = port_index;
    ev->protocol = model->get_urids().atom_eventTransfer;
    ev->size = sizeof(LV2_Atom) + size;

    LV2_Atom* atom = (LV2_Atom*)ev->body;
    atom->type = type;
    atom->size = size;

    if (zix_ring_write_space(_plugin_events) >= sizeof(evbuf) + size)
    {
        zix_ring_write(_plugin_events, evbuf, sizeof(evbuf));
        zix_ring_write(_plugin_events, (const char*)body, size);
        return true;
    }
    else
    {
        fprintf(stderr, "Plugin => UI buffer overflow!\n");
        return false;
    }
}

// TODO: Currently unused
bool Lv2_UI_IO::update(LV2Model* model)
{
//    /* Check quit flag and close if set. */
//    if (zix_sem_try_wait(&model->done)) {
//        jalv_close_ui(model);
//        return false;
//    }
//
//    /* Emit UI events. */
//    ControlChange ev;
//    const size_t  space = zix_ring_read_space(model->_plugin_events);
//    for (size_t i = 0;
//         i + sizeof(ev) < space;
//         i += sizeof(ev) + ev.size) {
//        /* Read event header to get the size */
//        zix_ring_read(model->_plugin_events, (char*)&ev, sizeof(ev));
//
//        /* Resize read buffer if necessary */
//        model->ui_event_buf = realloc(model->ui_event_buf, ev.size);
//        void* const buf = model->ui_event_buf;
//
//        /* Read event body */
//        zix_ring_read(model->_plugin_events, (char*)buf, ev.size);
//
//        if (model->opts.dump && ev.protocol == model->urids.atom_eventTransfer) {
//            /* Dump event in Turtle to the console */
//            LV2_Atom* atom = (LV2_Atom*)buf;
//            char*     str  = sratom_to_turtle(
//                    model->ui_sratom, &model->unmap, "LV2:", NULL, NULL,
//                    atom->type, atom->size, LV2_ATOM_BODY(atom));
//            jalv_ansi_start(stdout, 35);
//            printf("\n## Plugin => UI (%u bytes) ##\n%s\n", atom->size, str);
//            jalv_ansi_reset(stdout);
//            free(str);
//        }
//
//        ui_port_event(model, ev.index, ev.size, ev.protocol, buf);
//
//        if (ev.protocol == 0 && model->opts.print_controls) {
//            jalv_print_control(model, &model->ports[ev.index], *(float*)buf);
//        }
//    }

    return true;
}

// TODO: Currently unused!
ControlID* Lv2_UI_IO::control_by_symbol(LV2Model* model, const char* sym)
{
    for (size_t i = 0; i < model->controls.size(); ++i)
    {
        if (!strcmp(lilv_node_as_string(model->controls[i]->symbol), sym))
        {
            return model->controls[i].get();
        }
    }
    return nullptr;
}

} // namespace lv2
} // namespace sushi

#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

#include "lv2_ui_io.h"
#include "logging.h"
namespace sushi {
namespace lv2 {
MIND_GET_LOGGER;
//ProcessorReturnCode Lv2Wrapper::init(float /*sample_rate*/)
//{
//    /* The log print needs to be in a cpp file for initialisation order reasons */
//    SUSHI_LOG_ERROR("Sushi was not built with LV2 support!");
//    return ProcessorReturnCode::ERROR;
//}
}
}
#endif