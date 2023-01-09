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
 * @brief For signalling that Sushi should exit.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_EXIT_CONTROL_H
#define SUSHI_EXIT_CONTROL_H

#include <condition_variable>

extern bool exit_flag;
extern std::condition_variable exit_notifier;

bool exit_condition();

void signal_handler([[maybe_unused]] int sig);

void error_exit(const std::string& message);

#endif // SUSHI_EXIT_CONTROL_H
