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

#include "include/sushi/passive_factory.h"

#include "passive_factory_implementation.h"

namespace sushi
{

PassiveFactory::PassiveFactory()
{
    _implementation = std::make_unique<PassiveFactoryImplementation>();
}

PassiveFactory::~PassiveFactory()
{

}

std::pair<std::unique_ptr<Sushi>, Status> PassiveFactory::new_instance(SushiOptions& options)
{
    return _implementation->new_instance(options);
}

std::unique_ptr<RtController> PassiveFactory::rt_controller()
{
    return _implementation->rt_controller();
}

}