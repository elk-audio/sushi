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
* @brief Tools for Sushi if it is enclosed in a standalone host.
* @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#ifndef SUSHI_TERMINAL_UTILITIES_H
#define SUSHI_TERMINAL_UTILITIES_H

#include <vector>

#include "sushi.h"

namespace sushi {

enum class ParseStatus
{
    OK,
    ERROR,
    MISSING_ARGUMENTS,
    EXIT
};

void print_sushi_headline()
{
    std::cout << "SUSHI - Copyright 2017-2022 Elk, Stockholm" << std::endl;
    std::cout << "SUSHI is licensed under the Affero GPL 3.0. Source code is available at github.com/elk-audio" << std::endl;
}

void print_version_and_build_info()
{
    std::cout << "\nVersion " << CompileTimeSettings::sushi_version << std::endl;

    std::cout << "Build options enabled: ";

    for (const auto& o : CompileTimeSettings::enabled_build_options)
    {
        if (o != CompileTimeSettings::enabled_build_options.front())
        {
            std::cout << ", ";
        }

        std::cout << o;
    }

    std::cout << std::endl;

    std::cout << "Audio buffer size in frames: " << CompileTimeSettings::audio_chunk_size << std::endl;
    std::cout << "Git commit: " << CompileTimeSettings::git_commit_hash << std::endl;
    std::cout << "Built on: " << CompileTimeSettings::build_timestamp << std::endl;
}

ParseStatus parse_options(int argc, char* argv[], sushi::SushiOptions& options)
{
    optionparser::Stats cl_stats(usage, argc, argv);
    std::vector<optionparser::Option> cl_options(cl_stats.options_max);
    std::vector<optionparser::Option> cl_buffer(cl_stats.buffer_max);
    optionparser::Parser cl_parser(usage, argc, argv, &cl_options[0], &cl_buffer[0]);

    if (cl_parser.error())
    {
        return ParseStatus::ERROR;
    }

    if (cl_parser.optionsCount() == 0 || cl_options[OPT_IDX_HELP])
    {
        optionparser::printUsage(fwrite, stdout, usage);
        return ParseStatus::MISSING_ARGUMENTS;
    }

    for (int i = 0; i < cl_parser.optionsCount(); i++)
    {
        optionparser::Option& opt = cl_buffer[i];

        switch(opt.index())
        {
            case OPT_IDX_HELP:
            case OPT_IDX_UNKNOWN:
                // should be handled before arriving here
                assert(false);
                break;

            case OPT_IDX_VERSION:
                {
                    print_version_and_build_info();
                    return ParseStatus::EXIT;
                }
                break;

            case OPT_IDX_LOG_LEVEL:
                options.log_level.assign(opt.arg);
                break;

            case OPT_IDX_LOG_FILE:
                options.log_filename.assign(opt.arg);
                break;

            case OPT_IDX_LOG_FLUSH_INTERVAL:
                options.log_flush_interval = std::chrono::seconds(std::strtol(opt.arg, nullptr, 0));
                options.enable_flush_interval = true;
                break;

            case OPT_IDX_DUMP_PARAMETERS:
                options.enable_parameter_dump = true;
                break;

            case OPT_IDX_CONFIG_FILE:
                options.config_filename.assign(opt.arg);
                break;

            case OPT_IDX_USE_OFFLINE:
                options.frontend_type = FrontendType::OFFLINE;
                break;

            case OPT_IDX_INPUT_FILE:
                options.input_filename.assign(opt.arg);
                break;

            case OPT_IDX_OUTPUT_FILE:
                options.output_filename.assign(opt.arg);
                break;

            case OPT_IDX_USE_DUMMY:
                options.frontend_type = FrontendType::DUMMY;
                break;

            case OPT_IDX_USE_PORTAUDIO:
                options.frontend_type = FrontendType::PORTAUDIO;
                break;

            case OPT_IDX_AUDIO_INPUT_DEVICE:
                options.portaudio_input_device_id = atoi(opt.arg);
                break;

            case OPT_IDX_AUDIO_OUTPUT_DEVICE:
                options.portaudio_output_device_id = atoi(opt.arg);
                break;

            case OPT_IDX_USE_JACK:
                options.frontend_type = FrontendType::JACK;
                break;

            case OPT_IDX_CONNECT_PORTS:
                options.connect_ports = true;
                break;

            case OPT_IDX_JACK_CLIENT:
                options.jack_client_name.assign(opt.arg);
                break;

            case OPT_IDX_JACK_SERVER:
                options.jack_server_name.assign(opt.arg);
                break;

            case OPT_IDX_USE_XENOMAI_RASPA:
                options.frontend_type = FrontendType::XENOMAI_RASPA;
                break;

            case OPT_IDX_XENOMAI_DEBUG_MODE_SW:
                options.debug_mode_switches = true;
                break;

            case OPT_IDX_MULTICORE_PROCESSING:
                options.rt_cpu_cores = atoi(opt.arg);
                break;

            case OPT_IDX_TIMINGS_STATISTICS:
                options.enable_timings = true;
                break;

            case OPT_IDX_OSC_RECEIVE_PORT:
                options.osc_server_port = atoi(opt.arg);
                break;

            case OPT_IDX_OSC_SEND_PORT:
                options.osc_send_port = atoi(opt.arg);
                break;

            case OPT_IDX_GRPC_LISTEN_ADDRESS:
                options.grpc_listening_address = opt.arg;
                break;

            default:
                SushiArg::print_error("Unhandled option '", opt, "' \n");
                break;
        }
    }

    if (options.enable_parameter_dump == true)
    {
        options.frontend_type = FrontendType::DUMMY;
    }

    if (options.output_filename.empty() && !options.input_filename.empty())
    {
        options.output_filename = options.input_filename + "_proc.wav";
    }

    return ParseStatus::OK;
}

} // namespace Sushi

#endif // SUSHI_TERMINAL_UTILITIES_H
