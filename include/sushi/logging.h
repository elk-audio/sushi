/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Wrapper around spdlog and custom logging macros
 * *
 * If -DSUSHI_DISABLE_LOGGING is passed as a definition, all logging code
 * disappears without a trace. Useful for testing and outside releases.
 *
 * Usage:
 * Call ELKLOG_GET_LOGGER before any code in each file or preferably call
 * ELKLOG_GET_LOGGER_WITH_MODULE_NAME() to set a module name that will be
 * used by all log entries from that file.
 *
 * Write to the logger using the ELKLOG_LOG_XXX macros with cppformat style
 * ie: ELKLOG_LOG_INFO("Setting x to {} and y to {}", x, y);
 *
 * spdlog supports ostream style too, but that doesn't work with
 * SUSHI_DISABLE_LOGGING unfortunately
 *
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef LOGGING_H
#define LOGGING_H


#endif //LOGGING_H
