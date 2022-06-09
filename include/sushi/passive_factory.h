/*
* Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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

#ifndef PASSIVE_FACTORY_H
#define PASSIVE_FACTORY_H

#include "rt_controller.h"
#include "sushi_interface.h"

#include "factories/factory_base.h"
#include "real_time_controller.h"

namespace sushi_rpc {
class GrpcServer;
}

namespace sushi {

class Sushi;

namespace audio_frontend {
class PassiveFrontend;
}

namespace midi_frontend {
class PassiveMidiFrontend;
}

namespace engine {
class Transport;
}

/**
 * @brief Factory for when Sushi will be embedded into another audio host or into a plugin,
 *        and will only use Passive frontends for audio and MIDI.
 */
class PassiveFactory : public FactoryBase
{
public:
    PassiveFactory();
    ~PassiveFactory() override;

    std::unique_ptr<AbstractSushi> run(SushiOptions& options) override;

    /**
     * @brief Returns an instance of a RealTimeController, if run() completed successfully.
     *        If not, it returns an empty unique_ptr.
     * @return A unique_ptr with a RealTimeController, or not, depending on InitStatus.
     */
    std::unique_ptr<RealTimeController> rt_controller();

protected:
    InitStatus _setup_audio_frontend([[maybe_unused]] const SushiOptions& options,
                                     const jsonconfig::ControlConfig& config) override;

    InitStatus _set_up_midi([[maybe_unused]] const SushiOptions& options,
                            const jsonconfig::ControlConfig& config) override;

    InitStatus _set_up_control([[maybe_unused]] const SushiOptions& options,
                               [[maybe_unused]] jsonconfig::JsonConfigurator* configurator) override;

    InitStatus _load_json_events([[maybe_unused]] const SushiOptions& options,
                                 jsonconfig::JsonConfigurator* configurator) override;

private:
    std::unique_ptr<RealTimeController> _real_time_controller {nullptr};
};

} // namespace sushi


#endif // PASSIVE_FACTORY_H
