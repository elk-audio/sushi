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
 * @brief Public Sushi factory for standalone use.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "sushi/standalone_factory.h"

#include "standalone_factory_implementation.h"

namespace sushi {

StandaloneFactory::StandaloneFactory()
{
    _implementation = std::make_unique<internal::StandaloneFactoryImplementation>();
}

StandaloneFactory::~StandaloneFactory() = default;

std::pair<std::unique_ptr<Sushi>, Status> StandaloneFactory::new_instance(SushiOptions& options)
{
    return _implementation->new_instance(options);
}

}