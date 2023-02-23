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

/**
 * These three are used for signalling that Sushi should exit, from across threads.
 * In main(), Sushi waits on this condition variable to exit.
 */
extern bool exit_flag;
extern std::condition_variable exit_notifier;
bool exit_condition();

/**
 *  By invoking this method, you can signal to Sushi to exit,
 *  either through passing it to the standard signal(...) method,
 *  or through calling it directly in the code, e.g. when coming upon an unrecoverable error.
 *  When invoking this, Sushi will still wind down, cleanly close allocated resources, and flush logs.
 * @param sig the signal, e.g. SIGINT, SIGTERM.
 */
void exit_signal_handler([[maybe_unused]] int sig);

/**
 * If the error encountered is so severe as to require immediate exit, invoke this, instead of exit_signal_handler.
 * @param message
 */
void error_exit(const std::string& message);

#endif // SUSHI_EXIT_CONTROL_H
