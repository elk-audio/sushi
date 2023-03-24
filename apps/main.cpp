/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Sushi standalone application
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <iostream>
#include <csignal>
#include <condition_variable>

#include "sushi/logging.h"
#include "sushi/parameter_dump.h"
#include "sushi/portaudio_devices_dump.h"
#include "sushi/sushi.h"
#include "sushi/terminal_utilities.h"
#include "sushi/standalone_factory.h"
#include "sushi/offline_factory.h"

using namespace sushi;

SUSHI_GET_LOGGER_WITH_MODULE_NAME("main");

bool exit_flag = false;

bool exit_condition()
{
    return exit_flag;
}

std::condition_variable exit_notifier;

void signal_handler([[maybe_unused]] int sig)
{
    exit_flag = true;
    exit_notifier.notify_one();
}

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

int main(int argc, char* argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // option_parser accepts arguments excluding program name,
    // so skip it if it is present.
    if (argc > 0)
    {
        argc--;
        argv++;
    }

    SushiOptions options;

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

    if (options.enable_portaudio_devs_dump)
    {
#ifdef SUSHI_BUILD_WITH_PORTAUDIO
        std::cout << sushi::audio_frontend::generate_portaudio_devices_info_document() << std::endl;
        return 0;
#else
        std::cerr << "SUSHI not built with Portaudio support, cannot dump devices." << std::endl;
#endif
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

    SUSHI_LOG_INFO("Sushi exiting normally!");

    return 0;
}

std::unique_ptr<Sushi> start_sushi(SushiOptions options)
{
    std::unique_ptr<ActiveFactoryInterface> factory;

    if (options.frontend_type == FrontendType::DUMMY
        || options.frontend_type == FrontendType::OFFLINE)
    {
        factory = std::make_unique<OfflineFactory>();
    }
    else if (options.frontend_type == FrontendType::JACK
             || options.frontend_type == FrontendType::XENOMAI_RASPA
             || options.frontend_type == FrontendType::PORTAUDIO)
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