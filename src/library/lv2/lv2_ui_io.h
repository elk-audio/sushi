/**
 * @Brief UI IO Wrapper for VST 2.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_LV2_UI_IO_H
#define SUSHI_LV2_UI_IO_H

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2_evbuf.h"
#include "zix/ring.h"

namespace sushi {
namespace lv2 {

/**
 * @brief GUI IO code ported from Jalv. Not yet made to work, so subject to much refactoring still.
 */

class Lv2_UI_IO
{
public:
    MIND_DECLARE_NON_COPYABLE(Lv2_UI_IO)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */
    Lv2_UI_IO()
    {

    }

    ~Lv2_UI_IO()
    {
        if(_ui_events)
            zix_ring_free(_ui_events);

        if(_plugin_events)
            zix_ring_free(_plugin_events);
    }

    void init(float sample_rate, int _sample_rate);

    void write_ui_event(const char* buf);

    void set_buffer_size(uint32_t buffer_size);

    // Inherited from Jalv:

    void set_control(const ControlID* control, uint32_t size, LV2_URID type, const void* body);

    void ui_instantiate(LV2Model* model, const char* native_ui_type, void* parent);

    void ui_port_event(LV2Model* model,
                       uint32_t port_index,
                       uint32_t buffer_size,
                       uint32_t protocol,
                       const void* buffer);

    bool send_to_ui(LV2Model* model, uint32_t port_index, uint32_t type, uint32_t size, const void* body);

    bool update(LV2Model* model);

    void init_ui(LV2Model* model);

    uint32_t ui_port_index(void* const controller, const char* symbol);

    void apply_ui_events(LV2Model* model, uint32_t nframes);

    void ui_write(void* const jalv_handle, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, const void* buffer);

    bool ui_is_resizable(LV2Model* model);

    ControlID* control_by_symbol(LV2Model* model, const char* sym);

private:
    // void* window; ///< Window (if applicable)
    void*  ui_event_buf; ///< Buffer for reading UI port events

    uint32_t _buffer_size; ///< Plugin <= >UI communication buffer size
    double _update_rate;  ///< UI update rate in Hz

    uint32_t event_delta_t;  ///< Frames since last update sent to UI
    float ui_update_hz;   ///< Frequency of UI updates

    // TODO: use our own ringbuffer eventually.
    ZixRing* _ui_events; ///< Port events from UI
    ZixRing* _plugin_events; ///< Port events from plugin
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

    ProcessorReturnCode init(float sample_rate) override
    {
        return ProcessorReturnCode::ERROR;
    }

    void process_event(RtEvent /*event*/) override {}
    void process_audio(const ChunkSampleBuffer& /*in*/, ChunkSampleBuffer& /*out*/) override {}
};

}// end namespace lv2
}// end namespace sushi
#endif

#endif //SUSHI_LV2_UI_IO_H