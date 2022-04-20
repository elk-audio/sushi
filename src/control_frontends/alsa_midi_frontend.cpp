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
 * @brief Alsa midi frontend
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <chrono>
#include <cstdlib>

#include <alsa/seq_event.h>

#include "alsa_midi_frontend.h"
#include "library/midi_decoder.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("alsamidi");

namespace sushi {
namespace midi_frontend {

constexpr auto ALSA_POLL_TIMEOUT = std::chrono::milliseconds(200);
constexpr auto CLIENT_NAME = "Sushi";

int create_port(snd_seq_t* seq, int queue, const std::string& name, bool is_input)
{
    unsigned int capabilities;
    if (is_input)
    {
        capabilities = SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE;
    }
    else
    {
        capabilities = SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
    }

    int port = snd_seq_create_simple_port(seq, name.c_str(), capabilities, SND_SEQ_PORT_TYPE_APPLICATION);

    if (port < 0)
    {
        SUSHI_LOG_ERROR("Error opening ALSA MIDI port {}: {}", name, strerror(-port));
        return port;
    }

    /* For some weird reason directly creating the port with the specific timestamping
     * doesn't work, but we can set them once the port has been created */
    snd_seq_port_info_t* port_info;
    snd_seq_port_info_alloca(&port_info);
    snd_seq_get_port_info(seq, port, port_info);
    snd_seq_port_info_set_timestamp_queue(port_info, queue);
    snd_seq_port_info_set_timestamping(port_info, 1);
    snd_seq_port_info_set_timestamp_real(port_info, 1);
    int alsamidi_ret = snd_seq_set_port_info(seq, port, port_info);
    if (alsamidi_ret < 0)
    {
        SUSHI_LOG_ERROR("Couldn't set output port time configuration on port {}: {}", name, strerror(-alsamidi_ret));
        return alsamidi_ret;
    }
    SUSHI_LOG_INFO("Created Alsa Midi port {}", name);
    return port;
}

/**
 * @brief Filters midi messages currently not handled by sushi
 * @param type Alsa event type enumeration (see alsa/seq_event.h)
 * @return true if the event type corresponds to a midi message that should be handled by sushi
 *              false otherwise.
 */
bool is_midi_for_sushi(snd_seq_event_type_t type)
{
    if (type >= SND_SEQ_EVENT_NOTE && type <= SND_SEQ_EVENT_PITCHBEND)
    {
        return true;
    }
    return false;
}

AlsaMidiFrontend::AlsaMidiFrontend(int inputs, int outputs, midi_receiver::MidiReceiver* dispatcher)
        : BaseMidiFrontend(dispatcher),
          _inputs(inputs),
          _outputs(outputs)
{}

AlsaMidiFrontend::~AlsaMidiFrontend()
{
    stop();
    snd_midi_event_free(_input_parser);
    snd_midi_event_free(_output_parser);
    snd_seq_free_queue(_seq_handle, _queue);
    snd_seq_close(_seq_handle);
}

bool AlsaMidiFrontend::init()
{
    auto alsamidi_ret = snd_seq_open(&_seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (alsamidi_ret < 0)
    {
        SUSHI_LOG_ERROR("Error opening MIDI port: {}", strerror(-alsamidi_ret));
        return false;
    }

    alsamidi_ret = snd_seq_set_client_name(_seq_handle, CLIENT_NAME);
    if (alsamidi_ret < 0)
    {
        SUSHI_LOG_ERROR("Error setting client name: {}", strerror(-alsamidi_ret));
        return false;
    }

    _queue = snd_seq_alloc_queue(_seq_handle);

    alsamidi_ret = snd_seq_start_queue(_seq_handle, _queue, nullptr);
    if (alsamidi_ret < 0)
    {
        SUSHI_LOG_ERROR("Error setting up event queue {}", strerror(-alsamidi_ret));
        return false;
    }

    if (_init_ports() == false)
    {
        return false;
    }

    alsamidi_ret = snd_midi_event_new(ALSA_EVENT_MAX_SIZE, &_input_parser);
    if (alsamidi_ret < 0)
    {
        SUSHI_LOG_ERROR("Error creating MIDI Input RtEvent Parser: {}", strerror(-alsamidi_ret));
        return false;
    }
    alsamidi_ret = snd_midi_event_new(ALSA_EVENT_MAX_SIZE, &_output_parser);
    if (alsamidi_ret < 0)
    {
        SUSHI_LOG_ERROR("Error creating MIDI Output RtEvent Parser: {}", strerror(-alsamidi_ret));
        return false;
    }
    alsamidi_ret = snd_seq_nonblock(_seq_handle, 1);
    if (alsamidi_ret < 0)
    {
        SUSHI_LOG_ERROR("Setting non-blocking mode failed: {}", strerror(-alsamidi_ret));
        return false;
    }

    snd_midi_event_no_status(_input_parser, 1); /* Disable running status in the decoder */
    snd_midi_event_no_status(_output_parser, 1); /* Disable running status in the encoder */

    if (alsamidi_ret < 0 )
    {
        SUSHI_LOG_ERROR("Failed to set sequencer to use queue: {}", strerror(-alsamidi_ret));
        return false;
    }

    snd_seq_drain_output(_seq_handle);

    return _init_time();
}

void AlsaMidiFrontend::run()
{
    assert(_seq_handle);
    if (_inputs > 0)
    {
        _running = true;
        _worker = std::thread(&AlsaMidiFrontend::_poll_function, this);
    }
    SUSHI_LOG_INFO_IF(_inputs == 0, "No of midi inputs is 0, not starting read thread")
}

void AlsaMidiFrontend::stop()
{
    _running.store(false);
    if (_worker.joinable())
    {
        _worker.join();
    }
}

void AlsaMidiFrontend::_poll_function()
{
    auto descr_count = static_cast<nfds_t>(snd_seq_poll_descriptors_count(_seq_handle, POLLIN));
    auto descriptors = std::make_unique<pollfd[]>(descr_count);
    snd_seq_poll_descriptors(_seq_handle, descriptors.get(), static_cast<unsigned int>(descr_count), POLLIN);
    while (_running)
    {
        if (poll(descriptors.get(), descr_count, ALSA_POLL_TIMEOUT.count()) > 0)
        {
            snd_seq_event_t* ev{nullptr};
            uint8_t data_buffer[ALSA_EVENT_MAX_SIZE]{0};
            while (snd_seq_event_input(_seq_handle, &ev) > 0)
            {
                if (is_midi_for_sushi(ev->type))
                {
                    auto byte_count = snd_midi_event_decode(_input_parser, data_buffer, sizeof(data_buffer), ev);
                    if (byte_count > 0)
                    {
                        auto input = _port_to_input_map.find(ev->dest.port);
                        if (input != _port_to_input_map.end())
                        {
                            bool timestamped = (ev->flags | (SND_SEQ_TIME_STAMP_REAL & SND_SEQ_TIME_MODE_ABS)) == 1;
                            Time timestamp = timestamped ? _to_sushi_time(&ev->time.time) : IMMEDIATE_PROCESS;
                            _receiver->send_midi(input->second, midi::to_midi_data_byte(data_buffer, byte_count), timestamp);

                            SUSHI_LOG_DEBUG("Received midi message: [{:x} {:x} {:x} {:x}], port{}, timestamp: {}",
                                            data_buffer[0], data_buffer[1], data_buffer[2], data_buffer[3], input->second, timestamp.count());

                        }
                    }
                    SUSHI_LOG_WARNING_IF(byte_count < 0, "Decoder returned {}", strerror(-byte_count))
                }
                snd_seq_free_event(ev);
            }
        }
    }
}

void AlsaMidiFrontend::send_midi(int output, MidiDataByte data, [[maybe_unused]]Time timestamp)
{
    snd_seq_event ev;
    snd_seq_ev_clear(&ev);
    [[maybe_unused]] auto bytes = snd_midi_event_encode(_output_parser, data.data(), data.size(), &ev);

    SUSHI_LOG_INFO_IF(bytes <= 0, "Failed to encode event: {} {}", strerror(-bytes), ev.type)

    snd_seq_ev_set_source(&ev, _output_midi_ports[output]);
    snd_seq_ev_set_subs(&ev);
    snd_seq_real_time_t ev_time = {0,0}; //_to_alsa_time(timestamp); TODO: Find a proper solution for the midi sync.
    snd_seq_ev_schedule_real(&ev, _queue, false, &ev_time);
    bytes = snd_seq_event_output(_seq_handle, &ev);
    snd_seq_drain_output(_seq_handle);

    SUSHI_LOG_WARNING_IF(bytes <= 0, "Event output returned: {}, type {}", strerror(-bytes), ev.type)
}

bool AlsaMidiFrontend::_init_time()
{
    const snd_seq_real_time_t* start_time;
    snd_seq_queue_status_t* queue_status;
    snd_seq_queue_status_alloca(&queue_status);

    int alsamidi_ret = snd_seq_get_queue_status(_seq_handle, _queue, queue_status);
    if (alsamidi_ret < 0)
    {
        SUSHI_LOG_ERROR("Couldn't get queue status {}", strerror(-alsamidi_ret));
        return false;
    }
    start_time = snd_seq_queue_status_get_real_time(queue_status);
    _time_offset = get_current_time() - std::chrono::duration_cast<Time>(std::chrono::seconds(start_time->tv_sec) +
                                                                         std::chrono::nanoseconds(start_time->tv_nsec));
    return true;
}

bool AlsaMidiFrontend::_init_ports()
{
    bool add_index = _inputs > 1;
    for (int i = 0; i < _inputs; ++i)
    {
        int port = create_port(_seq_handle, _queue, "listen:in" + (add_index? "_" + std::to_string(i + 1) : ""), true);
        if (port < 0)
        {
            return false;
        }
        _input_midi_ports.push_back(port);
        _port_to_input_map[port] = i;
    }

    add_index = _outputs > 1;
    for (int i = 0; i < _outputs; ++i)
    {
        int port = create_port(_seq_handle, _queue, "read:out" + (add_index? "_" + std::to_string(i + 1) : ""), false);
        if (port < 0)
        {
            return false;
        }
        _output_midi_ports.push_back(port);
    }

    return true;
}

Time AlsaMidiFrontend::_to_sushi_time(const snd_seq_real_time_t* alsa_time)
{
    return std::chrono::duration_cast<Time>(std::chrono::seconds(alsa_time->tv_sec) +
                                            std::chrono::nanoseconds(alsa_time->tv_nsec)) + _time_offset;
}

snd_seq_real_time_t AlsaMidiFrontend::_to_alsa_time(Time timestamp)
{
    snd_seq_real_time alsa_time;
    auto offset_time =  timestamp - _time_offset;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(offset_time);
    alsa_time.tv_sec = static_cast<unsigned int>(seconds.count());
    alsa_time.tv_nsec = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::nanoseconds>(offset_time - seconds).count());
    return alsa_time;
}

} // end namespace midi_frontend
} // end namespace sushi

