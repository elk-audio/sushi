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

#ifndef SUSHI_STANDALONE_FACTORY_IMPLEMENTATION_H
#define SUSHI_STANDALONE_FACTORY_IMPLEMENTATION_H

#include "sushi/sushi.h"

#include "base_factory.h"

namespace sushi_rpc {
class GrpcServer;
}

namespace sushi::internal {

/**
 * @brief Factory for when Sushi will run in real-time standalone mode.
 */
class StandaloneFactoryImplementation : public BaseFactory
{
public:
    StandaloneFactoryImplementation();
    ~StandaloneFactoryImplementation() override;

    std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) override;

protected:
    Status _setup_audio_frontend(const SushiOptions& options,
                                 const jsonconfig::ControlConfig& config) override;

    Status _set_up_midi([[maybe_unused]] const SushiOptions& options,
                        const jsonconfig::ControlConfig& config) override;

    Status _set_up_control(const SushiOptions& options,
                           jsonconfig::JsonConfigurator* configurator) override;

    Status _load_json_events([[maybe_unused]] const SushiOptions& options,
                             jsonconfig::JsonConfigurator* configurator) override;
};

} // end namespace sushi::internal

#endif // SUSHI_STANDALONE_FACTORY_IMPLEMENTATION_H
