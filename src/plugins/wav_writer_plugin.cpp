#include "plugins/wav_writer_plugin.h"
#include "logging.h"

namespace sushi {
namespace wav_writer_plugin {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("wav_writer");

static const std::string DEFAULT_NAME = "sushi.testing.wav_writer";
static const std::string DEFAULT_LABEL = "Wav writer";

WavWriterPlugin::WavWriterPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _max_input_channels = N_AUDIO_CHANNELS;
    _max_output_channels = N_AUDIO_CHANNELS;
    _recording = register_bool_parameter("recording", "Recording", "bool", false);
}

WavWriterPlugin::~WavWriterPlugin()
{
    _stop_recording();
}

ProcessorReturnCode WavWriterPlugin::init(float sample_rate)
{
    memset(&_soundfile_info, 0, sizeof(_soundfile_info));
    // TODO: query from host but currently SUSHI might not report the right value here,
    // file needs to be open on samplerate / buffer update
    _soundfile_info.samplerate = sample_rate;
    _soundfile_info.channels = N_AUDIO_CHANNELS;
    _soundfile_info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_24);

    return ProcessorReturnCode::OK;
}

void WavWriterPlugin::configure(float sample_rate)
{
    _soundfile_info.samplerate = sample_rate;
}

void WavWriterPlugin::set_bypassed(bool bypassed)
{
    Processor::set_bypassed(bypassed);
}

void WavWriterPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
    case RtEventType::BOOL_PARAMETER_CHANGE:
    case RtEventType::FLOAT_PARAMETER_CHANGE:
    case RtEventType::INT_PARAMETER_CHANGE:
    {
        auto typed_event = event.parameter_change_event();
        if (typed_event->param_id() == 0)
        {
            if (typed_event->value())
            {
                _start_recording();
            }
            else
            {
                _stop_recording();
            }
        }
        break;
    }

    case RtEventType::ASYNC_WORK_NOTIFICATION:
    {
        auto typed_event = event.async_work_completion_event();
        if (typed_event->sending_event_id() == _pending_event_id &&
            typed_event->return_status() == WavWriterStatus::SUCCESS)
        {
            _pending_write_event = false;
        }
        break;
    }

    default:
        break;
    }
}

void WavWriterPlugin::process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    bypass_process(in_buffer, out_buffer);
    // Put samples in the ringbuffer already in interleaved format
    if (_recording->processed_value())
    {
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            for (int k = 0; k < N_AUDIO_CHANNELS; k++)
            {
                _ring_buffer.push(in_buffer.channel(k)[n]);
            }
        }
    }

    // Post RtEvent to write at an interval specified by WRITE_FREQUENCY
    if (_write_counter > WRITE_FREQUENCY)
    {
        _write_counter = 0;
        _post_write_event();
    }
    _write_counter++;
}

void WavWriterPlugin::_start_recording()
{
    _output_file = sf_open("./wav_writer_plug_out.wav", SFM_WRITE, &_soundfile_info);

    set_parameter_and_notify(_recording, true);
    _post_write_event();
}

void WavWriterPlugin::_stop_recording()
{
    set_parameter_and_notify(_recording, false);
    while (_pending_write_event); // busy wait
    sf_close(_output_file);
    _output_file = nullptr;
}

void WavWriterPlugin::_post_write_event()
{
    if (_pending_write_event == false)
    {
        auto e = RtEvent::make_async_work_event(&WavWriterPlugin::non_rt_callback, this->id(), this);
        _pending_event_id = e.async_work_event()->event_id();
        _pending_write_event = true;
        output_event(e);
    }
}

int WavWriterPlugin::_write_to_file()
{
    int samples_received = 0;

    float cur_sample;
    while (_ring_buffer.pop(cur_sample))
    {
        _file_buffer[samples_received++] = cur_sample;
    }
    sf_count_t samples_written = 0;
    while (samples_written < samples_received)
    {
        samples_written += sf_write_float(_output_file, &_file_buffer[samples_written],
                                            static_cast<sf_count_t>(samples_received + samples_written));
    }
    sf_write_sync(_output_file);
    return samples_written;
}

int WavWriterPlugin::_non_rt_callback(EventId id)
{
    if (id == _pending_event_id)
    {
        auto samples_written = _write_to_file();
        if (samples_written > 0)
        {
            SUSHI_LOG_INFO("WavWriter: Successfully wrote {} samples", samples_written);
            return WavWriterStatus::SUCCESS;
        }
        return WavWriterStatus::FAILURE;
    }
    else
    {
        SUSHI_LOG_WARNING("WavWriter: EventId of non-rt callback didn't match, {} vs {}", id, _pending_event_id);
        return WavWriterStatus::FAILURE;
    }
}

} // namespace wav_writer_plugin
} // namespace sushis
