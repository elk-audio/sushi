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

#include "src/factories/factory_base.h"
#include "real_time_controller.h"

namespace sushi
{

class Sushi;

namespace audio_frontend
{
class PassiveFrontend;
}

namespace midi_frontend
{
class PassiveMidiFrontend;
}

namespace engine
{
class Transport;
}

/**
 *
 */
class PassiveFactory : FactoryBase
{
public:
    PassiveFactory();
    ~PassiveFactory();

    void run(sushi::SushiOptions& options) override;

    std::unique_ptr<sushi::AbstractSushi> sushi() override;

    // TODO: This should be made inside of Factory / owned by Sushi preferably
    std::unique_ptr<sushi::RealTimeController> rt_controller();

    InitStatus sushi_init_status()
    {
        return _status;
    }

private:
    InitStatus _status {InitStatus::OK};

    std::unique_ptr<sushi::RealTimeController> _real_time_controller {nullptr};
};

} // namespace sushi


#endif // PASSIVE_FACTORY_H
