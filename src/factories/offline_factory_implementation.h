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
 * @brief Concrete implementation of the Sushi factory for offline use.
 *        This is a PIMPL class of sorts, used inside offline_factory.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_OFFLINE_FACTORY_H
#define SUSHI_OFFLINE_FACTORY_H

#include "sushi/sushi.h"

#include "base_factory.h"

namespace sushi::internal {

/**
 * @brief Factory for when Sushi is running in offline / dummy mode.
 */
class OfflineFactoryImplementation : public BaseFactory
{
public:
    OfflineFactoryImplementation();
    ~OfflineFactoryImplementation() override;

    std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) override;

protected:
    Status _setup_audio_frontend(const SushiOptions& options,
                                 const jsonconfig::ControlConfig& config) override;

    Status _set_up_midi([[maybe_unused]] const SushiOptions& options,
                        const jsonconfig::ControlConfig& config) override;

    Status _load_json_events([[maybe_unused]] const SushiOptions& options,
                             jsonconfig::JsonConfigurator* configurator) override;
};

} // end namespace sushi::internal

#endif // SUSHI_OFFLINE_FACTORY_H
