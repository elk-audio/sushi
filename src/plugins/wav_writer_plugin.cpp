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
            set_parameter_and_notify(_recording, typed_event->value());
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

    default:
        break;
    }
}

void WavWriterPlugin::process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    bypass_process(in_buffer, out_buffer);
    // Put samples in the ringbuffer already in interleaved format
    if (_recording->domain_value())
    {
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            for (int k = 0; k < N_AUDIO_CHANNELS; k++)
            {
                _ring_buffer.push(in_buffer.channel(k)[n]);
            }
        }
    }
}

void WavWriterPlugin::_start_recording()
{
    _output_file = sf_open("./wav_writer_plug_out.wav", SFM_WRITE, &_soundfile_info);

    _worker = std::thread(&WavWriterPlugin::_file_writer_task, this);
    _worker_running.store(true);
}

void WavWriterPlugin::_stop_recording()
{
    _worker_running.store(false);
    if (_worker.joinable())
    {
        _worker.join();
    }
    sf_close(_output_file);
}

void WavWriterPlugin::_file_writer_task()
{
    int samples_received = 0;

    while (_worker_running)
    {
        float cur_sample;
        while (_ring_buffer.pop(cur_sample))
        {
            _file_buffer[samples_received++] = cur_sample;
        }
        if (samples_received > (RINGBUFFER_SIZE/2))
        {
            sf_count_t samples_written = 0;
            while (samples_written < samples_received)
            {
                samples_written += sf_write_float(_output_file, &_file_buffer[samples_written],
                                                  static_cast<sf_count_t>(samples_received + samples_written));
            }
            sf_write_sync(_output_file);
            samples_received = 0;
        }
        std::this_thread::sleep_for(WORKER_SLEEP_INTERVAL);
    }

}


} // namespace wav_writer_plugin
} // namespace sushis
