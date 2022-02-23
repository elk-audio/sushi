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
* @brief Liblo OSC library wrapper
* @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/
#ifndef SUSHI_LIBLO_OSC_MESSENGER_H
#define SUSHI_LIBLO_OSC_MESSENGER_H

#include <sstream>

#include "osc_frontend.h"

#include "logging.h"

#include "osc_utils.h"

#include "lo/lo.h"
#include "lo/lo_types.h"

namespace sushi::osc {

class LibloOscMessenger : public BaseOscMessenger
{
public:
LibloOscMessenger(int receive_port, int send_port);

    ~LibloOscMessenger() override;

    bool init() override;

    int run() override;
    int stop() override;

    void* add_method(const char* address_pattern,
                     const char* type_tag_string,
                     OscMethodType type,
                     const void* connection) override;

    void delete_method(void* method) override;

    void send(const char* address_pattern, float payload) override;

    void send(const char* address_pattern, int payload) override;

private:
    lo_server_thread _osc_server {nullptr};

    lo_address _osc_out_address {nullptr};
};

} // namespace osc

#endif //SUSHI_LIBLO_OSC_MESSENGER_H
