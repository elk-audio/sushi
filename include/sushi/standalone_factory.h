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

#ifndef STANDALONE_FACTORY_H
#define STANDALONE_FACTORY_H

#include "factory_interface.h"
#include "sushi.h"

namespace sushi {

namespace internal {
    class StandaloneFactoryImplementation;
}

class StandaloneFactory : public FactoryInterface
{
public:
    StandaloneFactory();
    ~StandaloneFactory() override;

    [[nodiscard]] std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) override;

private:
    std::unique_ptr<internal::StandaloneFactoryImplementation> _implementation;
};

} // end namespace sushi

#endif // STANDALONE_FACTORY_H
