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
    _recording_parameter = register_bool_parameter("recording", "Recording", "bool", false);
    _write_speed_parameter = register_float_parameter("write_speed", "Write Speed", "writes/s",
                                                      DEFAULT_WRITE_INTERVAL,
                                                      MIN_WRITE_INTERVAL,
                                                      MAX_WRITE_INTERVAL);
    [[maybe_unused]] bool str_pr_ok = register_string_property("destination_file",
                                                               "Destination file",
                                                               "");

    assert(_recording_parameter && _write_speed_parameter && str_pr_ok);
}

WavWriterPlugin::~WavWriterPlugin()
{
    delete _destination_file_property;
}

ProcessorReturnCode WavWriterPlugin::init(float sample_rate)
{
    memset(&_soundfile_info, 0, sizeof(_soundfile_info));
    // TODO: query from host but currently SUSHI might not report the right value here,
    // file needs to be open on samplerate / buffer update
    _soundfile_info.samplerate = sample_rate;
    _soundfile_info.channels = N_AUDIO_CHANNELS;
    _soundfile_info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_24);
    _write_speed = _write_speed_parameter->domain_value();

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
    case RtEventType::STRING_PROPERTY_CHANGE:
    {
        auto typed_event = event.string_parameter_change_event();
        _destination_file_property = typed_event->value();
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
    if (_recording_parameter->processed_value())
    {
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            for (int k = 0; k < N_AUDIO_CHANNELS; k++)
            {
                _ring_buffer.push(in_buffer.channel(k)[n]);
            }
        }

    }

    // Post RtEvent to write at an interval specified by POST_WRITE_FREQUENCY
    if (_post_write_timer > POST_WRITE_FREQUENCY)
    {
        _post_write_event();
        _post_write_timer = 0;
    }
    _post_write_timer++;
}

WavWriterStatus WavWriterPlugin::_start_recording()
{
    unsigned int file_buffer_size = _soundfile_info.samplerate
                                  * _write_speed_parameter->descriptor()->max_domain_value()
                                  * _current_input_channels;

    if (_file_buffer.size() != file_buffer_size)
    {
        _file_buffer.resize(file_buffer_size);
    }

    // If no file name was passed. Set it to the default;
    if (_destination_file_property == nullptr)
    {
        _destination_file_property = new std::string(DEFAULT_PATH + this->name() + "_output");
    }
    std::string file_path = _available_path(*_destination_file_property);
    _output_file = sf_open(file_path.c_str(), SFM_WRITE, &_soundfile_info);
    if (_output_file == nullptr)
    {
        SUSHI_LOG_ERROR("libsndfile error: {}", sf_strerror(_output_file));
        return WavWriterStatus::FAILURE;
    }
    return WavWriterStatus::SUCCESS;
}

WavWriterStatus WavWriterPlugin::_stop_recording()
{
    _write_to_file(); // write any leftover samples
    int status = sf_close(_output_file);
    if (status != 0)
    {
        SUSHI_LOG_ERROR("libsndfile error: {}", sf_error_number(status));
        return WavWriterStatus::FAILURE;
    }
    _output_file = nullptr;
    return WavWriterStatus::SUCCESS;
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

    unsigned int write_limit = static_cast<int>(_write_speed * _soundfile_info.samplerate);
    if (_samples_received > write_limit || _recording_parameter->domain_value() == false)
    {
        while (_samples_written < _samples_received)
        {
            sf_count_t samples_to_write = static_cast<sf_count_t>(_samples_received - _samples_written);
            if (sf_error(_output_file) == 0)
            {
                _samples_written += sf_write_float(_output_file,
                                                  &_file_buffer[_samples_written],
                                                   samples_to_write);
            }
            else
            {
                SUSHI_LOG_ERROR("libsndfile: {}", sf_strerror(_output_file));
            }

        }
        sf_write_sync(_output_file);
        _samples_received = 0;
    }
    return _samples_written;
}

int WavWriterPlugin::_non_rt_callback(EventId /* id */)
{
    WavWriterStatus status = WavWriterStatus::SUCCESS;
    if (_recording_parameter->domain_value())
    {
        if (_output_file == nullptr)
        {
            // only change write speed before recording starts
            _write_speed = _write_speed_parameter->domain_value();
            status = _start_recording();
        }
        int samples_written = _write_to_file();
        if (samples_written > 0)
        {
            SUSHI_LOG_DEBUG("Sucessfully wrote {} samples", samples_written);
        }
    }
    else
    {
        if (_output_file)
        {
            status = _stop_recording();
        }
    }
    return status;
}

std::string WavWriterPlugin::_available_path(const std::string& requested_path)
{
    std::string suffix = ".wav";
    std::string new_path = requested_path + suffix;
    SF_INFO temp_info;
    SNDFILE* temp_file = sf_open(new_path.c_str(), SFM_READ, &temp_info);
    int suffix_counter = 1;
    while (sf_error(temp_file) == 0)
    {
        int status = sf_close(temp_file);
        if (status != 0)
        {
            SUSHI_LOG_ERROR("libsndfile error: {} {}",status, sf_error_number(status));
        }
        SUSHI_LOG_DEBUG("File {} already exists", new_path);
        new_path = requested_path + "_" + std::to_string(suffix_counter) + suffix;
        temp_file = sf_open(new_path.c_str(), SFM_READ, &temp_info);
        suffix_counter++;
    }
    int status = sf_close(temp_file);
    if (status != 0)
    {
        SUSHI_LOG_ERROR("libsndfile error: {}", sf_error_number(status));
    }
    return new_path;
}

} // namespace wav_writer_plugin
} // namespace sushis
