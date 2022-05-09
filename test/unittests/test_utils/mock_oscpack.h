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

#ifndef SUSHI_MOCK_OSCPACK_H
#define SUSHI_MOCK_OSCPACK_H

#include <gmock/gmock.h>

class IpEndpointName
{
public:
    static constexpr const char* ANY_ADDRESS = ""; // Unused - just a placeholder for mocking.

    IpEndpointName(const char* /*addressName*/, int /*port_*/ = 1234) {}
    virtual ~IpEndpointName() = default;
};

class PacketListener
{
public:
    PacketListener() = default;
    virtual ~PacketListener() = default;
};

class UdpTransmitSocket
{
public:
    UdpTransmitSocket() = default;
    explicit UdpTransmitSocket(const IpEndpointName& /*remoteEndpoint*/) {}

    virtual ~UdpTransmitSocket() = default;

    MOCK_METHOD(void,
                Send,
                (const char* data, std::size_t size),
                ());
};

class UdpListeningReceiveSocket
{
public:
    UdpListeningReceiveSocket() = default;
    UdpListeningReceiveSocket(const IpEndpointName& /*remoteEndpoint*/, PacketListener* /*listener*/) {}
    virtual ~UdpListeningReceiveSocket() = default;

    MOCK_METHOD(void,
                Run,
                (),
                ());

    MOCK_METHOD(void,
                AsynchronousBreak,
                (),
                ());
};

namespace osc
{

class ReceivedMessage;
struct MessageTerminator;
struct BeginMessage;

class OscPacketListener : public PacketListener
{
public:
    OscPacketListener() = default;
    virtual ~OscPacketListener() = default;

protected:
    virtual void ProcessMessage(const osc::ReceivedMessage& /*m*/, const IpEndpointName& /*remoteEndpoint*/) = 0;
};

}

#endif // SUSHI_MOCK_OSCPACK_H
