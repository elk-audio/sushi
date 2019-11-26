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
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cstdlib>

#include <alsa/seq_event.h>

#include "alsa_midi_frontend.h"
#include "library/midi_decoder.h"
#include "logging.h"

MIND_GET_LOGGER_WITH_MODULE_NAME("alsamidi");

namespace sushi {
namespace midi_frontend {

constexpr int ALSA_POLL_TIMEOUT_MS = 200;

AlsaMidiFrontend::AlsaMidiFrontend(midi_receiver::MidiReceiver* dispatcher)
        : BaseMidiFrontend(dispatcher)
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
        MIND_LOG_ERROR("Error opening MIDI port: {}", strerror(-alsamidi_ret));
        return false;
    }

    alsamidi_ret = snd_seq_set_client_name(_seq_handle, "Sushi");
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error setting client name: {}", strerror(-alsamidi_ret));
        return false;
    }

    _queue = snd_seq_alloc_queue(_seq_handle);

    alsamidi_ret = snd_seq_start_queue(_seq_handle, _queue, NULL);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error setting up event queue {}", strerror(-alsamidi_ret));
        return false;
    }

    if (_init_ports() == false)
    {
        return false;
    }

    alsamidi_ret = snd_midi_event_new(ALSA_EVENT_MAX_SIZE, &_input_parser);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error creating MIDI Input RtEvent Parser: {}", strerror(-alsamidi_ret));
        return false;
    }
    alsamidi_ret = snd_midi_event_new(ALSA_EVENT_MAX_SIZE, &_output_parser);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error creating MIDI Output RtEvent Parser: {}", strerror(-alsamidi_ret));
        return false;
    }
    alsamidi_ret = snd_seq_nonblock(_seq_handle, 1);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Setting non-blocking mode failed: {}", strerror(-alsamidi_ret));
        return false;
    }

    snd_midi_event_no_status(_input_parser, 1); /* Disable running status in the decoder */
    snd_midi_event_no_status(_output_parser, 1); /* Disable running status in the encoder */

    if (alsamidi_ret < 0 )
    {
        MIND_LOG_ERROR("Failed to set sequencer to use queue: {}", strerror(-alsamidi_ret));
        return false;
    }

    snd_seq_drain_output(_seq_handle);

    return _init_time();
}

void AlsaMidiFrontend::run()
{
    assert(_seq_handle);
    _running = true;
    _worker = std::thread(&AlsaMidiFrontend::_poll_function, this);
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
    nfds_t descr_count = static_cast<nfds_t>(snd_seq_poll_descriptors_count(_seq_handle, POLLIN));
    auto descriptors = std::make_unique<pollfd[]>(descr_count);
    snd_seq_poll_descriptors(_seq_handle, descriptors.get(), static_cast<unsigned int>(descr_count), POLLIN);
    while (_running)
    {
        if (poll(descriptors.get(), descr_count, ALSA_POLL_TIMEOUT_MS) > 0)
        {
            snd_seq_event_t* ev{nullptr};
            uint8_t data_buffer[ALSA_EVENT_MAX_SIZE]{0};
            while (snd_seq_event_input(_seq_handle, &ev) > 0)
            {
                // TODO - Consider if we should be filtering at all here or in the dispatcher instead
                if ((ev->type == SND_SEQ_EVENT_NOTEON)
                    || (ev->type == SND_SEQ_EVENT_NOTEOFF)
                    || (ev->type == SND_SEQ_EVENT_CONTROLLER)
                    || (ev->type == SND_SEQ_EVENT_PGMCHANGE)
                    || (ev->type == SND_SEQ_EVENT_PITCHBEND))
                {
                    auto byte_count = snd_midi_event_decode(_input_parser, data_buffer, sizeof(data_buffer), ev);
                    if (byte_count > 0)
                    {
                        bool timestamped = (ev->flags | (SND_SEQ_TIME_STAMP_REAL & SND_SEQ_TIME_MODE_ABS)) == 1;
                        Time timestamp = timestamped? _to_sushi_time(&ev->time.time) : IMMEDIATE_PROCESS;
                        _receiver->send_midi(0, midi::to_midi_data_byte(data_buffer, byte_count), timestamp);

                        MIND_LOG_DEBUG("Received midi message: [{:x} {:x} {:x} {:x}] timestamp: {}",
                                       data_buffer[0], data_buffer[1], data_buffer[2], data_buffer[3], timestamp.count());

                    }
                    MIND_LOG_WARNING_IF(byte_count < 0, "Decoder returned {}", byte_count);
                }
                snd_seq_free_event(ev);
            }
        }
    }
}

void AlsaMidiFrontend::send_midi(int /*output*/, MidiDataByte data, Time timestamp)
{
    snd_seq_event ev;
    snd_seq_ev_clear(&ev);
    auto bytes = snd_midi_event_encode(_output_parser, data.data(), data.size(), &ev);
    if (bytes <= 0 )
    {
        MIND_LOG_INFO("Failed to encode event: {} {}", strerror(-bytes), ev.type);
    }
    snd_seq_ev_set_source(&ev, _output_midi_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_real_time_t ev_time = _to_alsa_time(timestamp);
    snd_seq_ev_schedule_real(&ev, _queue, false, &ev_time);
    bytes = snd_seq_event_output(_seq_handle, &ev);
    snd_seq_drain_output(_seq_handle);
    if (bytes <= 0)
    {
        MIND_LOG_WARNING("Event output returned: {}, type {}", strerror(-bytes), ev.type);
    }
}


bool AlsaMidiFrontend::_init_time()
{
    const snd_seq_real_time_t* start_time;
    snd_seq_queue_status_t* queue_status;
    snd_seq_queue_status_alloca(&queue_status);

    int alsamidi_ret = snd_seq_get_queue_status(_seq_handle, _queue, queue_status);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Couldn't get queue status {}", strerror(-alsamidi_ret));
        return false;
    }
    start_time = snd_seq_queue_status_get_real_time(queue_status);
    _time_offset = get_current_time() - std::chrono::duration_cast<Time>(std::chrono::seconds(start_time->tv_sec) +
                                                                         std::chrono::nanoseconds(start_time->tv_nsec));
    return true;
}

bool AlsaMidiFrontend::_init_ports()
{
    _input_midi_port = snd_seq_create_simple_port(_seq_handle, "listen:in",
                                                  SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                  SND_SEQ_PORT_TYPE_APPLICATION);

    if (_input_midi_port < 0)
    {
        MIND_LOG_ERROR("Error opening ALSA MIDI port: {}", strerror(-_input_midi_port));
        return false;
    }

    _output_midi_port = snd_seq_create_simple_port(_seq_handle, "write:out",
                                                   SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                                                   SND_SEQ_PORT_TYPE_APPLICATION);

    if (_output_midi_port < 0)
    {
        MIND_LOG_ERROR("Error opening ALSA MIDI port: {}", strerror(-_output_midi_port));
        return false;
    }
    /* For some weird reason directly creating the port with the specific timestamping
     * doesn't work, but we can set them once the port has been created */
    snd_seq_port_info_t* port_info;
    snd_seq_port_info_alloca(&port_info);
    snd_seq_get_port_info(_seq_handle, _output_midi_port, port_info);
    snd_seq_port_info_set_timestamp_queue(port_info, _queue);
    snd_seq_port_info_set_timestamping(port_info, 1);
    snd_seq_port_info_set_timestamp_real(port_info, 1);
    int alsamidi_ret = snd_seq_set_port_info(_seq_handle, _output_midi_port, port_info);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Couldn't set output port time configuration{}", strerror(-alsamidi_ret));
        return false;
    }

    snd_seq_get_port_info(_seq_handle, _input_midi_port, port_info);
    snd_seq_port_info_set_timestamp_queue(port_info, _queue);
    snd_seq_port_info_set_timestamping(port_info, 1);
    snd_seq_port_info_set_timestamp_real(port_info, 1);
    alsamidi_ret = snd_seq_set_port_info(_seq_handle, _input_midi_port, port_info);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Couldn't set inpput port time configuration {}", strerror(-alsamidi_ret));
        return false;
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

