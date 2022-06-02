/*
* Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
*
* SUSHI is free software: you can redistribute it and/or modify it under the terms of
* the GNU Affero General Public License as published by the Free Software Foundation,
* either version 3 of the License, or (at your option) any later version.
*
* SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License along with
* SUSHI. If not, see http://www.gnu.org/licenses/
*/

#include "standalone_factory.h"

#include "logging.h"

#include "engine/audio_engine.h"
#include "src/sushi.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

namespace sushi
{

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sushi-factory");

StandaloneFactory::StandaloneFactory() = default;

StandaloneFactory::~StandaloneFactory() = default;

void StandaloneFactory::run(SushiOptions& options)
{
    sushi::init_logger(options);

}

std::unique_ptr<sushi::AbstractSushi> StandaloneFactory::sushi()
{

}

} // namespace sushi