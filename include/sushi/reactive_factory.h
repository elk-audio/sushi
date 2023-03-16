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

#ifndef REACTIVE_FACTORY_H
#define REACTIVE_FACTORY_H

#include "sushi.h"
#include "rt_controller.h"

namespace sushi
{

class ReactiveFactoryImplementation;

class ReactiveFactory
{
public:
    ReactiveFactory();
    ~ReactiveFactory();

    [[nodiscard]] std::pair<std::unique_ptr<Sushi>, Status> new_instance (SushiOptions& options);

    /**
     * @brief Returns an instance of a RealTimeController, if new_instance(...) completed successfully.
     *        If not, it returns an empty unique_ptr.
     * @return A unique_ptr with a RtController sub-class, or not, depending on InitStatus.
     */
    [[nodiscard]] std::unique_ptr<RtController> rt_controller();

private:
    std::unique_ptr<ReactiveFactoryImplementation> _implementation;
};

} // namespace sushi

#endif // REACTIVE_FACTORY_H
