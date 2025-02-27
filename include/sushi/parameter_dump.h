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
 * @brief Utility functions for writing parameter names to a file.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_PARAMETER_DUMP_H
#define SUSHI_PARAMETER_DUMP_H

#include "rapidjson/document.h"

namespace sushi {

namespace control {
    class SushiControl;
}

rapidjson::Document generate_processor_parameter_document(sushi::control::SushiControl* engine_controller);

} // end namespace sushi


#endif //SUSHI_PARAMETER_DUMP_H
