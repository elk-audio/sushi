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

#ifndef FACTORY_INTERFACE_H
#define FACTORY_INTERFACE_H

#include "sushi.h"

namespace sushi {

class FactoryInterface
{
public:
    FactoryInterface() = default;
    virtual ~FactoryInterface() = default;

    [[nodiscard]] virtual std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) = 0;
};

} // end namespace sushi

#endif // FACTORY_INTERFACE_H
