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
 * @brief Utility functions for dumping Portaudio devices info
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "rapidjson/document.h"

namespace sushi {

/**
 * @brief Retrieve Portaudio's registered devices information.
 *        Can be queried before instantiating an actual PortaudioFrontend
 *
 * @return Device information list in JSON format
 */
rapidjson::Document generate_portaudio_devices_info_document();

} // end namespace sushi

