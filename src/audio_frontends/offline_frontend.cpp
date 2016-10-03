#include <logging.h>
#include "offline_frontend.h"

namespace sushi {

namespace audio_frontend {

MIND_GET_LOGGER;

OfflineFrontend::~OfflineFrontend()
{
    cleanup();
    delete _file_buffer;
}

AudioFrontendInitStatus OfflineFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendInitStatus::OK)
    {
        return ret_code;
    }

    auto off_config = static_cast<OfflineFrontendConfiguration*>(_config);

    // Open audio file and check channels / sample rate
    memset(&_soundfile_info, 0, sizeof(_soundfile_info));
    if (! (_input_file = sf_open(off_config->_input_filename.c_str(), SFM_READ, &_soundfile_info)) )
    {
        cleanup();
        MIND_LOG_ERROR("Unable to open input file {}", off_config->_input_filename);
        return AudioFrontendInitStatus::INVALID_INPUT_FILE;
    }
    if (static_cast<unsigned int>(_soundfile_info.channels) != _config->_n_channels)
    {
        MIND_LOG_ERROR("Mismatch in number of channels of audio file, which is {}", _soundfile_info.channels);
        cleanup();
        return AudioFrontendInitStatus::INVALID_N_CHANNELS;
    }
    auto sample_rate_file = static_cast<unsigned int>(_soundfile_info.samplerate);
    if (sample_rate_file != _config->_sample_rate)
    {
        MIND_LOG_WARNING("Sample rate mismatch between file ({}) and engine ({})",
                         sample_rate_file,
                         _config->_sample_rate);
    }

    // Open output file with same format as input file
    if (! (_output_file = sf_open(off_config->_output_filename.c_str(), SFM_WRITE, &_soundfile_info)) )
    {
        cleanup();
        MIND_LOG_ERROR("Unable to open output file {}", off_config->_output_filename);
        return AudioFrontendInitStatus::INVALID_OUTPUT_FILE;
    }

    // Initialize buffers
    _file_buffer = new float(config->_n_channels * AUDIO_CHUNK_SIZE);

    return ret_code;
}

void OfflineFrontend::cleanup()
{
    sf_close(_input_file);
    sf_close(_output_file);
}

void OfflineFrontend::run()
{
    int readcount;
    while ( (readcount = static_cast<int>(sf_readf_float(_input_file, _file_buffer, AUDIO_CHUNK_SIZE))) )
    {
        _buffer.input_from_interleaved(_file_buffer);
        _engine->process_chunk(&_buffer);
        _buffer.output_to_interleaved(_file_buffer);

        // Should we check the number of samples effectively written?
        // Not done in libsndfile's example
        sf_writef_float(_output_file, _file_buffer, readcount);
    }
}


}; // end namespace audio_frontend

}; // end namespace sushi
