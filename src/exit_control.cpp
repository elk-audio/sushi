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

#include "exit_control.h"

#include <iostream>

bool exit_flag = false;
std::condition_variable exit_notifier;

bool exit_condition()
{
    return exit_flag;
}

void exit_on_signal([[maybe_unused]] int sig)
{
    exit_flag = true;
    exit_notifier.notify_one();
}

void error_exit(const std::string& message)
{
    std::cerr << message << std::endl;
    std::exit(1);
}

