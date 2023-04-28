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
* @brief OSC utilities
* @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/
#ifndef SUSHI_OSC_UTILS_H
#define SUSHI_OSC_UTILS_H

#include <sstream>
#include "osc_frontend.h"
#include "logging.h"

namespace sushi {
namespace osc
{

enum class OscMethodType
{
    SEND_PARAMETER_CHANGE_EVENT,
    SEND_PROPERTY_CHANGE_EVENT,
    SEND_BYPASS_STATE_EVENT,
    SEND_KEYBOARD_NOTE_EVENT,
    SEND_KEYBOARD_MODULATION_EVENT,
    SEND_PROGRAM_CHANGE_EVENT,
    SET_TEMPO,
    SET_TIME_SIGNATURE,
    SET_PLAYING_MODE,
    SET_TEMPO_SYNC_MODE,
    SET_TIMING_STATISTICS_ENABLED,
    RESET_TIMING_STATISTICS,
    NONE
};

class BaseOscMessenger
{
public:
    BaseOscMessenger(int receive_port,
                     int send_port,
                     const std::string& send_ip) : _receive_port(receive_port),
                                                   _send_port(send_port),
                                                   _send_ip(send_ip)
    {}

    virtual ~BaseOscMessenger() = default;

    /**
     * Call before using the class instance.
     * @return Whether initialization succeeded.
     */
    virtual bool init() = 0;

    /**
     * Starts the OSC messaging thread.
     */
    virtual void run() = 0;

    /**
     * Stops the OSC messaging thread.
     */
    virtual void stop() = 0;

    /**
     * Subscribes to callbacks, triggered when the specified OSC
     * "Address Pattern" and "Type Tag String" combination are received.
     * @param address_pattern The Address Pattern to look for
     * @param type_tag_string  The Type Tag String to look for
     * @param type The OscMethodType enum - specifying what category the message is.
     *             Necessary if the OSC implementation uses a different callback for each type.
     * @param callback_data A void* of "user_data", which can vary depending on the callback_data type.
     *                      The actual payload need only be known in the corresponding callback passed.
     * @return A unique handle which identifies the added method.
     */
    virtual void* add_method(const char* address_pattern,
                             const char* type_tag_string,
                             OscMethodType type,
                             const void* callback_data) = 0;

    /**
     * Deletes the connection to a specific callback, created with add_method.
     * This method signature uses a void* handle, so that it can be compatible with many OSC libraries.
     * But if we will only ever use oscpack, it can be changed to uint_64_t.
     * @param handle Needs to be the void* returned from add_method.
     */
    virtual void delete_method(void* handle) = 0;

    /**
     * Send a single OSC message. Currently only TTS: "i" or "f" are supported.
     * @param address_pattern  The address pattern to send to.
     * @param payload The values of the message.
     */
    virtual void send(const char* address_pattern, int payload) = 0;
    virtual void send(const char* address_pattern, float payload) = 0;
    virtual void send(const char* address_pattern, const std::string& payload) = 0;

    std::string send_ip() const
    {
        return _send_ip;
    }

    int send_port() const
    {
        return _send_port;
    }

    int receive_port() const
    {
        return _receive_port;
    }

protected:
    int _receive_port;
    int _send_port;
    std::string _send_ip;

    std::atomic_bool _osc_initialized {false};
};

/**
 * @brief Ensure that a string is safe to use as an OSC path by stripping illegal
 *        characters and replacing spaces with underscores.
 * @param name The string to process
 * @return an std::string safe to use as an OSC path
 */
std::string make_safe_path(std::string name);

} // namespace osc
} // namespace sushi
#endif //SUSHI_OSC_UTILS_H
