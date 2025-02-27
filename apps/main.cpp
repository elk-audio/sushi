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
 * @brief Main entry point to Sushi
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <iostream>
#include <csignal>
#include <condition_variable>

#include "elklog/static_logger.h"

#include "sushi/utils.h"
#include "sushi/parameter_dump.h"
#include "sushi/portaudio_devices_dump.h"
#include "sushi/coreaudio_devices_dump.h"
#include "sushi/sushi.h"
#include "sushi/terminal_utilities.h"
#include "sushi/standalone_factory.h"
#include "sushi/offline_factory.h"

using namespace sushi;

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("main");

/**
 * These are used for signalling that Sushi should exit, from across threads.
 * In main(), Sushi waits on this condition variable to exit.
 */
bool exit_flag = false;
std::condition_variable exit_notifier;

bool exit_condition()
{
    return exit_flag;
}

/**
 *  By invoking this method, you can signal to Sushi to exit,
 *  either through passing it to the standard signal(...) method,
 *  or through calling it directly in the code, e.g. when coming upon an unrecoverable error.
 *  When invoking this, Sushi will still wind down, cleanly close allocated resources, and flush logs.
 * @param sig the signal, e.g. SIGINT, SIGTERM.
 */
void exit_on_signal([[maybe_unused]] int sig)
{
    exit_flag = true;
    exit_notifier.notify_one();
}

/**
 * If the error encountered is so severe as to require immediate exit, invoke this, instead of exit_on_signal.
 * @param message
 */
void error_exit(const std::string& message, sushi::Status status)
{
    std::cerr << message << std::endl;
    int error_code = static_cast<int>(status);
    std::exit(error_code);
}

/**
 * Tries to start Sushi
 * @param options a SushiOptions structure
 * @return a Sushi instance, if successful - if not, the unique_ptr is empty.
 */
std::unique_ptr<Sushi> start_sushi(SushiOptions options);

void pipe_signal_handler([[maybe_unused]] int sig)
{
    ELKLOG_LOG_INFO("Pipe signal received and ignored: {}", sig);
}

static void print_sushi_headline()
{
    std::cout << "SUSHI - Copyright 2017-2023 Elk Audio AB, Stockholm" << std::endl;
    std::cout << "SUSHI is licensed under the Affero GPL 3.0. Source code is available at github.com/elk-audio" << std::endl;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, exit_on_signal);
    signal(SIGTERM, exit_on_signal);
#ifndef _MSC_VER
    signal(SIGPIPE, pipe_signal_handler);
#endif
    
    // option_parser accepts arguments excluding program name,
    // so skip it if it is present.
    if (argc > 0)
    {
        argc--;
        argv++;
    }

    SushiOptions options;

    options.config_source = sushi::ConfigurationSource::FILE;

    auto option_status = parse_options(argc, argv, options);
    if (option_status == ParseStatus::ERROR)
    {
        return 1;
    }
    else if (option_status == ParseStatus::MISSING_ARGUMENTS)
    {
        return 2;
    }
    else if (option_status == ParseStatus::EXIT)
    {
        return 0;
    }

    init_logger(options);

    if (options.enable_audio_devices_dump)
    {
        if (options.frontend_type == FrontendType::PORTAUDIO)
        {
#ifdef SUSHI_BUILD_WITH_PORTAUDIO
            std::cout << sushi::generate_portaudio_devices_info_document() << std::endl;
            return 0;
#else
            std::cerr << "SUSHI not built with Portaudio support, cannot dump devices." << std::endl;
#endif
        }
        else if (options.frontend_type == FrontendType::APPLE_COREAUDIO)
        {
#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO
            std::cout << sushi::generate_coreaudio_devices_info_document() << std::endl;
#else
            std::cerr << "SUSHI not built with Apple CoreAudio support, cannot dump devices." << std::endl;
#endif
            return 0;
        }
        else
        {
            std::cout << "No frontend specified or specified frontend not supported (please specify ." << std::endl;
            return 1;
        }
    }

    auto sushi = start_sushi(options);

    if (sushi == nullptr)
    {
        return -1;
    }

    if (options.frontend_type != FrontendType::OFFLINE)
    {
        std::mutex m;
        std::unique_lock<std::mutex> lock(m);
        exit_notifier.wait(lock, exit_condition);
    }

    sushi->stop();

    ELKLOG_LOG_INFO("Sushi exiting normally!");

    return 0;
}

std::unique_ptr<Sushi> start_sushi(SushiOptions options)
{
    std::unique_ptr<FactoryInterface> factory;

    if (options.frontend_type == FrontendType::DUMMY ||
        options.frontend_type == FrontendType::OFFLINE)
    {
        factory = std::make_unique<OfflineFactory>();
    }
    else if (options.frontend_type == FrontendType::JACK ||
             options.frontend_type == FrontendType::XENOMAI_RASPA ||
             options.frontend_type == FrontendType::APPLE_COREAUDIO ||
             options.frontend_type == FrontendType::PORTAUDIO)
    {
        factory = std::make_unique<StandaloneFactory>();
    }
    else
    {
        error_exit("Invalid frontend configuration. Reactive, or None, are not supported when standalone.",
                   Status::FRONTEND_IS_INCOMPATIBLE_WITH_STANDALONE);
    }

    // Initialising:

    auto [sushi, status] = factory->new_instance(options);

    if (status == Status::FAILED_OSC_FRONTEND_INITIALIZATION)
    {
        error_exit("Instantiating OSC server on port " + std::to_string(options.osc_server_port) + " failed.",
                   status);
    }
    else if (status != Status::OK)
    {
        auto message = to_string(status);
        if (status == Status::FAILED_INVALID_FILE_PATH)
        {
            message.append(options.config_filename);
        }

        error_exit(message, status);
    }

    if (options.enable_parameter_dump)
    {
        std::cout << sushi::generate_processor_parameter_document(sushi->controller());
        std::cout << "Parameter dump completed - exiting." << std::endl;
        std::exit(EXIT_SUCCESS);
    }
    else
    {
        print_sushi_headline();
    }

    // ...and starting:

    auto start_status = sushi->start();

    if (start_status == Status::OK)
    {
        return std::move(sushi);
    }
    else if (start_status == Status::FAILED_TO_START_RPC_SERVER)
    {
        sushi.reset();

        error_exit("Failure starting gRPC server on address " + options.grpc_listening_address, status);
    }

    return nullptr;
}