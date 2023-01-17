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

#include "concrete_sushi.h"

#include "include/sushi/terminal_utilities.h"

#include "library/parameter_dump.h"
#include "compile_time_settings.h"

#include "factories/standalone_factory.h"
#include "factories/offline_factory.h"

#include "logging.h"
#include "audio_frontends/portaudio_devices_dump.h"

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

void error_exit(const std::string& message)
{
    std::cerr << message << std::endl;
    std::exit(1);
}

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
        std::exit(0);
#else
        std::cerr << "SUSHI not built with Portaudio support, cannot dump devices." << std::endl;
#endif
    }

    std::unique_ptr<BaseFactory> factory;

    if (options.frontend_type == FrontendType::DUMMY ||
        options.frontend_type == FrontendType::PASSIVE)
    {
        factory = std::make_unique<OfflineFactory>();
    }
    else if (options.frontend_type == FrontendType::JACK ||
             options.frontend_type == FrontendType::XENOMAI_RASPA ||
             options.frontend_type == FrontendType::PORTAUDIO)
    {
        factory = std::make_unique<StandaloneFactory>();
    }
    else
    {
        // Invalid frontend configuration.
        // Passive, or None, are not supported when standalone.
        return 3;
    }

    auto [sushi, init_status] = factory->new_instance(options);

    if (init_status != InitStatus::OK)
    {
        auto message = to_string(init_status);
        if (init_status == InitStatus::FAILED_INVALID_FILE_PATH)
        {
            message.append(options.config_filename);
        }

        error_exit(message);
    }

    if (options.enable_parameter_dump)
    {
        std::cout << sushi::generate_processor_parameter_document(sushi->controller());
        std::cout << "Parameter dump completed - exiting." << std::endl;
        std::exit(0);
    }
    else
    {
        print_sushi_headline();
    }

    sushi->start();

    if (options.frontend_type != FrontendType::OFFLINE)
    {
        std::mutex m;
        std::unique_lock<std::mutex> lock(m);
        exit_notifier.wait(lock, exit_condition);
    }

    sushi->exit();

    SUSHI_LOG_INFO("Sushi exiting normally!");

    return 0;
}
