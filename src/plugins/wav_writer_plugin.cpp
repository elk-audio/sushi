#include "plugins/wav_writer_plugin.h"
#include "logging.h"

namespace sushi {
namespace wav_writer_plugin {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("wav_writer");

static const std::string DEFAULT_NAME = "sushi.testing.wav_writer";
static const std::string DEFAULT_LABEL = "Wav writer";
static const std::string DEFAULT_PATH = "./";

WavWriterPlugin::WavWriterPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _max_input_channels = N_AUDIO_CHANNELS;
    _max_output_channels = N_AUDIO_CHANNELS;
    _recording = register_bool_parameter("recording", "Recording", "bool", false);
    _write_speed = register_float_parameter("write_speed", "Write Speed", "writes/s",
                                            DEFAULT_WRITE_INTERVAL,
                                            MIN_WRITE_INTERVAL,
                                            MAX_WRITE_INTERVAL);
    register_string_property("destination_file", "Destination file", "path");
}

WavWriterPlugin::~WavWriterPlugin()
{
    if (_recording->domain_value())
    {
        _stop_recording();
    }
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
        if (typed_event->param_id() == _recording->descriptor()->id())
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
        else
        {
            // Write speed can't be updated while recording
            if (_recording->domain_value() == false)
            {
                InternalPlugin::process_event(event);
            }
        }
        break;
    }

    case RtEventType::STRING_PROPERTY_CHANGE:
    {
        auto typed_event = event.string_parameter_change_event();
        if (*typed_event->value() != _destination_file_property)
        {
            _stop_recording();
            _destination_file_property = *typed_event->value();
        }
        break;
    }

    default:
        InternalPlugin::process_event(event);
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

        // Post RtEvent to write at an interval specified by WRITE_FREQUENCY
        if (_flushed_samples_counter > FLUSH_FREQUENCY)
        {
            _post_write_event();
            _flushed_samples_counter = 0;
        }
        _flushed_samples_counter += AUDIO_CHUNK_SIZE;
    }
}

void WavWriterPlugin::_start_recording()
{
    if (_destination_file_property.empty())
    {
        _destination_file_property = DEFAULT_PATH + this->name() + "_output.wav";
    }
    _file_buffer.resize(_soundfile_info.samplerate
                        * _write_speed->descriptor()->max_domain_value()
                        * _current_input_channels);

    _output_file = sf_open(_destination_file_property.c_str(), SFM_WRITE, &_soundfile_info);
    set_parameter_and_notify(_recording, true);
    _post_write_event();
}

void WavWriterPlugin::_stop_recording()
{
    set_parameter_and_notify(_recording, false);
    _write_to_file(); // write any leftover samples
    sf_close(_output_file);
    _output_file = nullptr;
}

void WavWriterPlugin::_post_write_event()
{
    auto e = RtEvent::make_async_work_event(&WavWriterPlugin::non_rt_callback, this->id(), this);
    _pending_event_id = e.async_work_event()->event_id();
    output_event(e);
}

int WavWriterPlugin::_write_to_file()
{
    float cur_sample;
    while (_ring_buffer.pop(cur_sample))
    {
        if (_samples_received > _file_buffer.size())
        {
            _file_buffer.push_back(cur_sample);
        }
        else
        {
            _file_buffer[_samples_received++] = cur_sample;
        }
    }

    _samples_written = 0;

    unsigned int write_limit = static_cast<int>(_write_speed->domain_value() * _soundfile_info.samplerate);
    if (_samples_received > write_limit || _recording->domain_value() == false)
    {
        while (_samples_written < _samples_received)
        {
            _samples_written += sf_write_float(_output_file, &_file_buffer[_samples_written],
                                               static_cast<sf_count_t>(_samples_received - _samples_written));
        }
        sf_write_sync(_output_file);
        _samples_received = 0;
    }
    return _samples_written;
}

int WavWriterPlugin::_non_rt_callback(EventId id)
{
    if (id == _pending_event_id)
    {
        int samples_written = _write_to_file();
        if (samples_written > 0)
        {
            SUSHI_LOG_DEBUG("Successfully wrote {} samples", samples_written);
        }
        return WavWriterStatus::SUCCESS;
    }
    else
    {
        SUSHI_LOG_WARNING("EventId of non-rt callback didn't match, {} vs {}", id, _pending_event_id);
        return WavWriterStatus::FAILURE;
    }
}

} // namespace wav_writer_plugin
} // namespace sushis
