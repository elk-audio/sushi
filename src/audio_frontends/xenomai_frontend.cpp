#ifdef SUSHI_BUILD_WITH_XENOMAI
#include <deque>
#include <unistd.h>
#include <chrono>

#include "logging.h"
#include "xenomai_frontend.h"
#include "library/random_note_player.h"

namespace sushi {

namespace audio_frontend {

MIND_GET_LOGGER;



DiskIoHandler::~DiskIoHandler()
{
    stop();
    if (_input_file)
    {
        sf_close(_input_file);
    }
    if (_output_file)
    {
        sf_close(_output_file);
    }
    if (_in_file_buffer)
    {
        delete _in_file_buffer;
    }
    if (_out_file_buffer)
    {
        delete _out_file_buffer;
    }
}


AudioFrontendStatus DiskIoHandler::init(const std::string& input_filename, const std::string& output_filename)
{
    memset(&_soundfile_info, 0, sizeof(_soundfile_info));
    if (! (_input_file = sf_open(input_filename.c_str(), SFM_READ, &_soundfile_info)) )
    {
        MIND_LOG_ERROR("Unable to open input file {}", input_filename);
        return AudioFrontendStatus::INVALID_INPUT_FILE;
    }
    // Open output file with same format as input file
    if (! (_output_file = sf_open(output_filename.c_str(), SFM_WRITE, &_soundfile_info)) )
    {
        MIND_LOG_ERROR("Unable to open output file {}", output_filename);
        return AudioFrontendStatus::INVALID_OUTPUT_FILE;
    }
    _in_file_buffer = new float[_soundfile_info.channels * AUDIO_CHUNK_SIZE];
    _out_file_buffer = new float[_soundfile_info.channels * AUDIO_CHUNK_SIZE];
    return AudioFrontendStatus::OK;
}


bool DiskIoHandler::run()
{
    if (!_input_file || !_output_file || _running)
    {
        return false;
    }
    _running.store(true);
    _io_thread = std::thread(&DiskIoHandler::worker, this);
    return true;
}


void DiskIoHandler::stop()
{
    _running.store(false);
    if (_io_thread.joinable())
    {
        _io_thread.join();
    }
}


void DiskIoHandler::worker()
{
    ChunkSampleBuffer sample_buffer(_soundfile_info.channels);
    bool can_send = true;
    bool end_of_file = false;
    // read the first frame to get the initial conditions right (and assume we have at least 64 frames in the file)
    sf_readf_float(_input_file, _in_file_buffer, static_cast<sf_count_t>(AUDIO_CHUNK_SIZE));
    while (_running.load())
    {
        auto start_time = std::chrono::system_clock::now();
        // Read from file and fill the buffer until it says no.
        while (can_send && !end_of_file)
        {
            sample_buffer.from_interleaved(_in_file_buffer);
            can_send = _out_queue->push(sample_buffer);
            if (!can_send)
            {
                can_send = true;
                break; // And retry send at next wakeup;
            }
            if (sf_readf_float(_input_file, _in_file_buffer, static_cast<sf_count_t>(AUDIO_CHUNK_SIZE)) == 0)
            {
                end_of_file = true;
            }
        }
        // Write incoming file buffers to disk
        while (!_in_queue->isEmpty())
        {
            auto buffer = _in_queue->pop();
            if (buffer.valid)
            {
                buffer.item.to_interleaved(_out_file_buffer);
                sf_writef_float(_output_file, _out_file_buffer, static_cast<sf_count_t>(AUDIO_CHUNK_SIZE));
            }
        }
        std::this_thread::sleep_until(start_time + DISK_IO_PERIODICITY);
    }
}


AudioFrontendStatus XenomaiFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }
    auto cf = static_cast<XenomaiFrontendConfiguration*>(config);
    //_osc_control = std::make_unique<control_frontend::OSCFrontend>(&_event_queue);
    return _disk_io.init(cf->input_file, cf->output_file);
}


AudioFrontendStatus XenomaiFrontend::add_sequencer_events_from_json_def(const Json::Value &events)
{
    if (events.isArray())
    {
        _event_queue.reserve(events.size());
        for(const Json::Value& e : events)
        {
            int sample = static_cast<int>( std::round(e["time"].asDouble() * static_cast<double>(_engine->sample_rate()) ) );
            auto data = e["data"];
            if (e["type"] == "parameter_change")
            {
                _event_queue.push_back(std::make_tuple(sample,
                                                       new ParameterChangeEvent(EventType::FLOAT_PARAMETER_CHANGE,
                                                                                data["stompbox_instance"].asString(),
                                                                                sample % AUDIO_CHUNK_SIZE,
                                                                                data["parameter_id"].asString(),
                                                                                data["value"].asFloat())));

            }
            else if (e["type"] == "note_on")
            {
                _event_queue.push_back(std::make_tuple(sample,
                                                       new KeyboardEvent(EventType::NOTE_ON,
                                                                         data["stompbox_instance"].asString(),
                                                                         sample % AUDIO_CHUNK_SIZE,
                                                                         data["note"].asInt(),
                                                                         data["velocity"].asFloat())));
            }
            else if (e["type"] == "note_off")
            {
                _event_queue.push_back(std::make_tuple(sample,
                                                       new KeyboardEvent(EventType::NOTE_OFF,
                                                                         data["stompbox_instance"].asString(),
                                                                         sample % AUDIO_CHUNK_SIZE,
                                                                         data["note"].asInt(),
                                                                         data["velocity"].asFloat())));
            }
        }

        // Sort events by reverse time (lambda function compares first tuple element)
        std::sort(std::begin(_event_queue), std::end(_event_queue),
                  [](std::tuple<int, BaseEvent*> const &t1,
                     std::tuple<int, BaseEvent*> const &t2)
                  {
                      return std::get<0>(t1) >= std::get<0>(t2);
                  }
        );
    }
    else
    {
        MIND_LOG_ERROR("Invalid format for events in configuration file");
        return AudioFrontendStatus ::INVALID_SEQUENCER_DATA;
    }

    return AudioFrontendStatus ::OK;
}


void XenomaiFrontend::cleanup()
{}


void XenomaiFrontend::run()
{
    bool disk_io = _disk_io.run();
    if (!disk_io)
    {
        MIND_LOG_ERROR("Couldn't start disk streaming");
        return;
    }

    RT_TASK processing_task;
    rt_task_create(&processing_task, "ProcessingTask", 0x8000, 80, 0);
    int64_t period_ns = AUDIO_CHUNK_SIZE * 1000000000l / _disk_io.samplerate();
    MIND_LOG_INFO("Setting periodic task every {} ms", period_ns / 1000000.0f);
    rt_task_set_periodic(&processing_task, TM_NOW, period_ns);
    rt_task_start(&processing_task, xenomai_callback_generator, static_cast<void*>(this));

    //_osc_control->run();
    sleep(30);
    rt_task_set_periodic(&processing_task, TM_NOW, TM_INFINITE);
    rt_task_join(&processing_task);
    // Leave some time for the disk streaming to finish.
    sleep(1);
    rt_task_delete(&processing_task);
}


int XenomaiFrontend::internal_process_callback()
{
    if (_in_audio_queue.isEmpty())
    {
        return -1;
    }
    _samplecount += AUDIO_CHUNK_SIZE;
    while ( !_event_queue.empty() && (std::get<0>(_event_queue.back()) < _samplecount) )
    {
        auto next_event = _event_queue.back();
        auto plugin_event = std::get<1>(next_event);
        _engine->send_rt_event(std::get<1>(next_event));

        _event_queue.pop_back();
        // TODO don't delete things in the audio thread.
        delete plugin_event;
    }

    auto in_buffer = _in_audio_queue.pop();
    _out_buffer.clear();
    _engine->process_chunk(&in_buffer.item, &_out_buffer);
    _out_audio_queue.push(_out_buffer);
    return 0;
}


/*
 * Xenomai task launch function
 */
void xenomai_callback_generator(void* data)
{
    auto frontend = static_cast<XenomaiFrontend*>(data);
    uint64_t underruns;
    while (true)
    {
        int status = rt_task_wait_period(&underruns);
        if (status != 0)
        {
            MIND_LOG_WARNING("Xenomai underrun: {}", underruns);
        }
        frontend->rt_process_callback();
    }
}

}; // end namespace audio_frontend
}; // end namespace sushi

#endif
#ifndef SUSHI_BUILD_WITH_XENOMAI
#include <cassert>
#include "audio_frontends/xenomai_frontend.h"
#include "logging.h"
namespace sushi {
namespace audio_frontend {
MIND_GET_LOGGER;
XenomaiFrontend::XenomaiFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    MIND_LOG_ERROR("Sushi was not built with Xenomai support!");
    assert(false);
}}}
#endif