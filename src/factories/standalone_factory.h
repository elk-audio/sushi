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

#ifndef SUSHI_STANDALONE_FACTORY_H
#define SUSHI_STANDALONE_FACTORY_H

#include "include/sushi/sushi.h"

#include "src/factories/base_factory.h"

namespace sushi_rpc {
class GrpcServer;
}

namespace sushi {

/**
 * @brief Factory for when Sushi will run in real-time standalone mode.
 */
class StandaloneFactory : public BaseFactory
{
public:
    StandaloneFactory();
    ~StandaloneFactory() override;

    std::unique_ptr<Sushi> run(SushiOptions& options) override;

protected:
    InitStatus _setup_audio_frontend(const SushiOptions& options,
                                     const jsonconfig::ControlConfig& config) override;

    InitStatus _set_up_midi([[maybe_unused]] const SushiOptions& options,
                            const jsonconfig::ControlConfig& config) override;

    InitStatus _set_up_control(const SushiOptions& options,
                               jsonconfig::JsonConfigurator* configurator) override;

    InitStatus _load_json_events([[maybe_unused]] const SushiOptions& options,
                                 jsonconfig::JsonConfigurator* configurator) override;
};

} // namespace sushi

#endif //SUSHI_STANDALONE_FACTORY_H
