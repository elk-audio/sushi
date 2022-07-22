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
* @brief OSCPACK OSC library wrapper
* @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/
#ifndef SUSHI_OSCPACK_OSC_MESSENGER_H
#define SUSHI_OSCPACK_OSC_MESSENGER_H

#include <sstream>
#include <iostream>

#include "osc_frontend.h"
#include "logging.h"
#include "osc_utils.h"

#include "third-party/oscpack/osc/OscReceivedElements.h"
#include "third-party/oscpack/osc/OscOutboundPacketStream.h"

#ifndef OSCPACK_UNIT_TESTS
#include "third-party/oscpack/osc/OscPacketListener.h"
#include "third-party/oscpack/ip/UdpSocket.h"
#endif

namespace oscpack = ::osc;

namespace sushi::osc
{

struct LightKey
{
    LightKey(const char* first_, const char* second_): first(first_), second(second_) {}
    const char* first;
    const char* second;
};

bool operator<(const std::pair<std::string, std::string>& fat, const LightKey& light);
bool operator<(const LightKey& light, const std::pair<std::string, std::string>& fat);

// In OscPack they have an incoming packet size max of 4098.
// But that's already allocated when we receive it - they seem to not enforce any size for
// sending.
// 1512 is the common default MTU - UDP headers are 8 bytes fixed size, giving he below.
constexpr size_t OSC_OUTPUT_BUFFER_SIZE = 1504;

// We need to be able to cast between OSC_CALLBACK_HANDLE, and void*, to keep the API in BaseOscMessenger consistent.
// If we are OK with breaking the API compatibility with Liblo, we can just change the API to directly expose uint64_t.
#include <cstdint>
#if INTPTR_MAX == INT32_MAX
using OSC_CALLBACK_HANDLE = uint32_t;
#elif INTPTR_MAX == INT64_MAX
using OSC_CALLBACK_HANDLE = uint64_t;
#else
#error "Environment not 32 or 64-bit."
#endif

class OscpackOscMessenger : public BaseOscMessenger,
                            public oscpack::OscPacketListener
{
public:
    OscpackOscMessenger(int receive_port, int send_port, const std::string& send_ip);

    ~OscpackOscMessenger() override;

    bool init() override;

    void run() override;
    void stop() override;

    void* add_method(const char* address_pattern,
                     const char* type_tag_string,
                     OscMethodType type,
                     const void* callback_data) override;

    void delete_method(void* handle) override;

    void send(const char* address_pattern, float payload) override;

    void send(const char* address_pattern, int payload) override;

    void send(const char* address_pattern, const std::string& payload) override;

protected:
    /**
     * Defined in osc::OscPacketListener.
     */
    void ProcessMessage(const oscpack::ReceivedMessage& m, const IpEndpointName& /*remoteEndpoint*/) override;

private:
    void _osc_receiving_worker();

    void _send_parameter_change_event(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _send_property_change_event(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _send_bypass_state_event(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _send_keyboard_note_event(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _send_keyboard_modulation_event(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _send_program_change_event(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _set_timing_statistics_enabled(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _reset_timing_statistics(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _set_tempo(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _set_time_signature(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _set_playing_mode(const oscpack::ReceivedMessage& m, void* user_data) const;
    void _set_tempo_sync_mode(const oscpack::ReceivedMessage& m, void* user_data) const;

    std::thread _osc_receive_worker;
    std::unique_ptr<UdpTransmitSocket> _transmit_socket {nullptr};

    std::unique_ptr<UdpListeningReceiveSocket> _receive_socket {nullptr};

    struct MessageRegistration
    {
        void* callback_data {nullptr};
        OscMethodType type {OscMethodType::NONE};
        OSC_CALLBACK_HANDLE handle {0};
    };

    std::map<std::pair<std::string, std::string>, MessageRegistration, std::less<>> _registered_messages;

    OSC_CALLBACK_HANDLE _last_generated_handle {0};

    char _output_buffer[OSC_OUTPUT_BUFFER_SIZE];
};

} // namespace osc

#endif //SUSHI_OSCPACK_OSC_MESSENGER_H
