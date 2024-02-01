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
 * @brief Utility functions around rapidjson library
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_UTILS_H
#define SUSHI_UTILS_H

#include <iostream>
#include <optional>

#include "rapidjson/document.h"
#include "sushi/utils.h"

namespace sushi {

struct SushiOptions;

std::ostream& operator<<(std::ostream& out, const rapidjson::Document& document);

/**
 * Reads the file passed in through the path argument.
 * Returns true, and the contents on read success.
 * Or false and an empty string on failure.
 * @param path The absolut path to the file.
 * @return file content on success - nullopt on failure
 */
std::optional<std::string> read_file(const std::string& path);

/**
 * This should be called only once in the lifetime of the embedding binary - or it will fail.
 * @param options
 */
void init_logger([[maybe_unused]] const SushiOptions& options);

}

#endif // SUSHI_UTILS_H

