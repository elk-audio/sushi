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

#ifndef REACTIVE_FACTORY_H
#define REACTIVE_FACTORY_H

#include "factory_interface.h"
#include "sushi.h"

namespace sushi {

namespace internal {
    class ReactiveFactoryImplementation;
}

class RtController;

class ReactiveFactory : public FactoryInterface
{
public:
    ReactiveFactory();
    ~ReactiveFactory() override;

    [[nodiscard]] std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) override;

    /**
     * @brief Returns an instance of a RealTimeController, if new_instance(...) completed successfully.
     *        If not, it returns an empty unique_ptr.
     * @return A unique_ptr with a RtController sub-class, or not, depending on InitStatus.
     */
    [[nodiscard]] std::unique_ptr<RtController> rt_controller();

private:
    std::unique_ptr<internal::ReactiveFactoryImplementation> _implementation;
};

} // end namespace sushi

#endif // REACTIVE_FACTORY_H
