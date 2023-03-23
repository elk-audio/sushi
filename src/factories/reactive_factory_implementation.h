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

#ifndef REACTIVE_FACTORY_IMPLEMENTATION_H
#define REACTIVE_FACTORY_IMPLEMENTATION_H

#include "rt_controller.h"
#include "sushi.h"

#include "base_factory.h"

namespace sushi_rpc {
class GrpcServer;
}

namespace sushi {

class ConcreteSushi;

namespace audio_frontend {
class ReactiveFrontend;
}

namespace midi_frontend {
class ReactiveMidiFrontend;
}

namespace engine {
class Transport;
}

/**
 * @brief Factory for when Sushi will be embedded into another audio host or into a plugin,
 *        and will only use Reactive frontends for audio and MIDI.
 */
class ReactiveFactoryImplementation : public BaseFactory
{
public:
    ReactiveFactoryImplementation();
    ~ReactiveFactoryImplementation() override;

    std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) override;

    /**
     * @brief Returns an instance of a RealTimeController, if new_instance(...) completed successfully.
     *        If not, it returns an empty unique_ptr.
     * @return A unique_ptr with a RtController sub-class, or not, depending on InitStatus.
     */
    std::unique_ptr<RtController> rt_controller();

protected:
    Status _setup_audio_frontend([[maybe_unused]] const SushiOptions& options,
                                 const jsonconfig::ControlConfig& config) override;

    Status _set_up_midi([[maybe_unused]] const SushiOptions& options,
                        const jsonconfig::ControlConfig& config) override;

    Status _set_up_control([[maybe_unused]] const SushiOptions& options,
                           [[maybe_unused]] jsonconfig::JsonConfigurator* configurator) override;

    Status _load_json_events([[maybe_unused]] const SushiOptions& options,
                             jsonconfig::JsonConfigurator* configurator) override;

private:
    std::unique_ptr<RtController> _real_time_controller {nullptr};
};

} // namespace sushi


#endif // REACTIVE_FACTORY_IMPLEMENTATION_H
