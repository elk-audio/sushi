/*
 * Copyright 2017-2023 Elk Audio AB
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

/**
 * @brief Public Sushi factory for reactive use.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "sushi/reactive_factory.h"
#include "sushi/rt_controller.h"

#include "reactive_factory_implementation.h"

namespace sushi
{

ReactiveFactory::ReactiveFactory()
{
    _implementation = std::make_unique<internal::ReactiveFactoryImplementation>();
}

ReactiveFactory::~ReactiveFactory() = default;

std::pair<std::unique_ptr<Sushi>, Status> ReactiveFactory::new_instance(SushiOptions& options)
{
    return _implementation->new_instance(options);
}

std::unique_ptr<RtController> ReactiveFactory::rt_controller()
{
    return _implementation->rt_controller();
}

}