/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Main entry point to Sushi
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <csignal>
#include <memory>
#include <condition_variable>

#include "twine/src/twine_internal.h"

#include "logging.h"
#include "options.h"
#include "generated/version.h"
#include "engine/audio_engine.h"
#include "audio_frontends/offline_frontend.h"
#include "audio_frontends/jack_frontend.h"
#include "audio_frontends/xenomai_raspa_frontend.h"
#include "engine/json_configurator.h"
#include "control_frontends/osc_frontend.h"
#include "control_frontends/alsa_midi_frontend.h"
#include "library/parameter_dump.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

enum class FrontendType
{
    OFFLINE,
    DUMMY,
    JACK,
    XENOMAI_RASPA,
    NONE
};

constexpr std::array SUSHI_ENABLED_BUILD_OPTIONS = {
#ifdef SUSHI_BUILD_WITH_VST2
        "vst2",
#endif
#ifdef SUSHI_BUILD_WITH_VST3
        "vst3",
#endif
#ifdef SUSHI_BUILD_WITH_LV2
        "lv2",
#endif
#ifdef SUSHI_BUILD_WITH_JACK
        "jack",
#endif
#ifdef SUSHI_BUILD_WITH_XENOMAI
        "xenomai",
#endif
#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
        "rpc control",
#endif
#ifdef SUSHI_BUILD_WITH_ABLETON_LINK
        "ableton link",
#endif
};

bool                    exit_flag = false;
bool                    exit_condition() {return exit_flag;}
std::condition_variable exit_notifier;

void sigint_handler([[maybe_unused]] int sig)
{
    exit_flag = true;
    exit_notifier.notify_one();
}

void print_sushi_headline()
{
    std::cout << "SUSHI - Copyright 2017-2020 Elk, Stockholm" << std::endl;
    std::cout << "SUSHI is licensed under the Affero GPL 3.0. Source code is available at github.com/elk-audio" << std::endl;
}

void error_exit(const std::string& message)
{
    std::cerr << message << std::endl;
    std::exit(1);
}

void print_version_and_build_info()
{
    std::cout << "\nVersion "   << SUSHI__VERSION_MAJ << "."
                                << SUSHI__VERSION_MIN << "."
                                << SUSHI__VERSION_REV << std::endl;

    std::cout << "Build options enabled: ";
    for (const auto& o : SUSHI_ENABLED_BUILD_OPTIONS)
    {
        if (o != SUSHI_ENABLED_BUILD_OPTIONS.front())
        {
           std::cout << ", ";
        }
        std::cout << o;
    }
    std::cout << std::endl;

    std::cout << "Audio buffer size in frames: " << AUDIO_CHUNK_SIZE << std::endl;
    std::cout << "Git commit: " << SUSHI_GIT_COMMIT_HASH << std::endl;
    std::cout << "Built on: " << SUSHI_BUILD_TIMESTAMP << std::endl;
}

int main(int argc, char* argv[])
{
#ifdef SUSHI_BUILD_WITH_XENOMAI
    auto ret = sushi::audio_frontend::XenomaiRaspaFrontend::global_init();
    if (ret < 0)
    {
        error_exit("Failed to initialize Xenomai process, err. code: " + std::to_string(ret));
    }
#endif

    signal(SIGINT, sigint_handler);

    ////////////////////////////////////////////////////////////////////////////////
    // Command Line arguments parsing
    ////////////////////////////////////////////////////////////////////////////////

    // option_parser accepts arguments excluding program name,
    // so skip it if it is present
    if (argc > 0)
    {
        argc--;
        argv++;
    }

    optionparser::Stats cl_stats(usage, argc, argv);
    std::vector<optionparser::Option> cl_options(cl_stats.options_max);
    std::vector<optionparser::Option> cl_buffer(cl_stats.buffer_max);
    optionparser::Parser cl_parser(usage, argc, argv, &cl_options[0], &cl_buffer[0]);

    if (cl_parser.error())
    {
        return 1;
    }
    if (cl_parser.optionsCount() == 0 || cl_options[OPT_IDX_HELP])
    {
        optionparser::printUsage(fwrite, stdout, usage);
        return 0;
    }

    std::string input_filename;
    std::string output_filename;

    std::string log_level = std::string(SUSHI_LOG_LEVEL_DEFAULT);
    std::string log_filename = std::string(SUSHI_LOG_FILENAME_DEFAULT);
    std::string config_filename = std::string(SUSHI_JSON_FILENAME_DEFAULT);
    std::string jack_client_name = std::string(SUSHI_JACK_CLIENT_NAME_DEFAULT);
    std::string jack_server_name = std::string("");
    int osc_server_port = SUSHI_OSC_SERVER_PORT;
    int osc_send_port = SUSHI_OSC_SEND_PORT;
    std::string grpc_listening_address = std::string(SUSHI_GRPC_LISTENING_PORT);
    FrontendType frontend_type = FrontendType::NONE;
    bool connect_ports = false;
    bool debug_mode_switches = false;
    int  rt_cpu_cores = 1;
    bool enable_timings = false;
    bool enable_flush_interval = false;
    bool enable_parameter_dump = false;
    std::chrono::seconds log_flush_interval = std::chrono::seconds(0);

    for (int i=0; i<cl_parser.optionsCount(); i++)
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
                std::exit(1);
            }
            break;

        case OPT_IDX_LOG_LEVEL:
            log_level.assign(opt.arg);
            break;

        case OPT_IDX_LOG_FILE:
            log_filename.assign(opt.arg);
            break;

        case OPT_IDX_LOG_FLUSH_INTERVAL:
            log_flush_interval = std::chrono::seconds(std::strtol(opt.arg, nullptr, 0));
            enable_flush_interval = true;
            break;

        case OPT_IDX_DUMP_PARAMETERS:
            enable_parameter_dump = true;
            break;

        case OPT_IDX_CONFIG_FILE:
            config_filename.assign(opt.arg);
            break;

        case OPT_IDX_USE_OFFLINE:
            frontend_type = FrontendType::OFFLINE;
            break;

        case OPT_IDX_INPUT_FILE:
            input_filename.assign(opt.arg);
            break;

        case OPT_IDX_OUTPUT_FILE:
            output_filename.assign(opt.arg);
            break;

        case OPT_IDX_USE_DUMMY:
            frontend_type = FrontendType::DUMMY;
            break;

        case OPT_IDX_USE_JACK:
            frontend_type = FrontendType::JACK;
            break;

        case OPT_IDX_CONNECT_PORTS:
            connect_ports = true;
            break;

        case OPT_IDX_JACK_CLIENT:
            jack_client_name.assign(opt.arg);
            break;

        case OPT_IDX_JACK_SERVER:
            jack_server_name.assign(opt.arg);
            break;

        case OPT_IDX_USE_XENOMAI_RASPA:
            frontend_type = FrontendType::XENOMAI_RASPA;
            break;

        case OPT_IDX_XENOMAI_DEBUG_MODE_SW:
            debug_mode_switches = true;
            break;

        case OPT_IDX_MULTICORE_PROCESSING:
            rt_cpu_cores = atoi(opt.arg);
            break;

        case OPT_IDX_TIMINGS_STATISTICS:
            enable_timings = true;
            break;

        case OPT_IDX_OSC_RECEIVE_PORT:
            osc_server_port = atoi(opt.arg);
            break;

        case OPT_IDX_OSC_SEND_PORT:
            osc_send_port = atoi(opt.arg);
            break;

        case OPT_IDX_GRPC_LISTEN_ADDRESS:
            grpc_listening_address = opt.arg;
            break;

        default:
            SushiArg::print_error("Unhandled option '", opt, "' \n");
            break;
        }
    }

    if (enable_parameter_dump == false)
    {
        print_sushi_headline();
    }
    else
    {
        frontend_type = FrontendType::DUMMY;
    }

    if (output_filename.empty() && !input_filename.empty())
    {
        output_filename = input_filename + "_proc.wav";
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Logger configuration
    ////////////////////////////////////////////////////////////////////////////////
    auto ret_code = SUSHI_INITIALIZE_LOGGER(log_filename, "Logger", log_level, enable_flush_interval, log_flush_interval);
    if (ret_code != SUSHI_LOG_ERROR_CODE_OK)
    {
        std::cerr << SUSHI_LOG_GET_ERROR_MESSAGE(ret_code) << ", using default." << std::endl;
    }

    SUSHI_GET_LOGGER_WITH_MODULE_NAME("main");

    ////////////////////////////////////////////////////////////////////////////////
    // Main body //
    ////////////////////////////////////////////////////////////////////////////////

    if (frontend_type == FrontendType::XENOMAI_RASPA)
    {
        twine::init_xenomai(); // must be called before setting up any worker pools
    }
    auto engine = std::make_unique<sushi::engine::AudioEngine>(SUSHI_SAMPLE_RATE_DEFAULT, rt_cpu_cores);
    auto midi_dispatcher = std::make_unique<sushi::midi_dispatcher::MidiDispatcher>(engine.get());
    auto configurator = std::make_unique<sushi::jsonconfig::JsonConfigurator>(engine.get(),
                                                                              midi_dispatcher.get(),
                                                                              config_filename);

    std::unique_ptr<sushi::midi_frontend::BaseMidiFrontend>                 midi_frontend;
    std::unique_ptr<sushi::control_frontend::OSCFrontend>                   osc_frontend;
    std::unique_ptr<sushi::audio_frontend::BaseAudioFrontend>               audio_frontend;
    std::unique_ptr<sushi::audio_frontend::BaseAudioFrontendConfiguration>  frontend_config;

    auto [audio_config_status, audio_config] = configurator->load_audio_config();
    if (audio_config_status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        if (audio_config_status == sushi::jsonconfig::JsonConfigReturnStatus::INVALID_FILE)
        {
            error_exit("Error reading config file, invalid file: " + config_filename);
        }
        error_exit("Error reading host config, check logs for details.");
    }
    int cv_inputs = audio_config.cv_inputs.value_or(0);
    int cv_outputs = audio_config.cv_outputs.value_or(0);
    int midi_inputs = audio_config.midi_inputs.value_or(1);
    int midi_outputs = audio_config.midi_outputs.value_or(1);

    midi_dispatcher->set_midi_inputs(midi_inputs);
    midi_dispatcher->set_midi_outputs(midi_outputs);

    ////////////////////////////////////////////////////////////////////////////////
    // Set up Audio Frontend //
    ////////////////////////////////////////////////////////////////////////////////

    switch (frontend_type)
    {
        case FrontendType::JACK:
        {
            SUSHI_LOG_INFO("Setting up Jack audio frontend");
            frontend_config = std::make_unique<sushi::audio_frontend::JackFrontendConfiguration>(jack_client_name,
                                                                                                 jack_server_name,
                                                                                                 connect_ports,
                                                                                                 cv_inputs,
                                                                                                 cv_outputs);
            audio_frontend = std::make_unique<sushi::audio_frontend::JackFrontend>(engine.get());
            break;
        }

        case FrontendType::XENOMAI_RASPA:
        {
            SUSHI_LOG_INFO("Setting up Xenomai RASPA frontend");
            frontend_config = std::make_unique<sushi::audio_frontend::XenomaiRaspaFrontendConfiguration>(debug_mode_switches,
                                                                                                         cv_inputs,
                                                                                                         cv_outputs);
            audio_frontend = std::make_unique<sushi::audio_frontend::XenomaiRaspaFrontend>(engine.get());
            break;
        }

        case FrontendType::DUMMY:
        case FrontendType::OFFLINE:
        {
            bool dummy = false;
            if (frontend_type == FrontendType::DUMMY)
            {
                dummy = true;
                SUSHI_LOG_INFO("Setting up dummy audio frontend");
            }
            else
            {
                SUSHI_LOG_INFO("Setting up offline audio frontend");
            }
            frontend_config = std::make_unique<sushi::audio_frontend::OfflineFrontendConfiguration>(input_filename,
                                                                                                    output_filename,
                                                                                                    dummy,
                                                                                                    cv_inputs,
                                                                                                    cv_outputs);
            audio_frontend = std::make_unique<sushi::audio_frontend::OfflineFrontend>(engine.get());
            break;
        }

        default:
            error_exit("No audio frontend selected.");
    }

    auto audio_frontend_status = audio_frontend->init(frontend_config.get());
    if (audio_frontend_status != sushi::audio_frontend::AudioFrontendStatus::OK)
    {
        error_exit("Error initializing frontend, check logs for details.");
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Load Configuration //
    ////////////////////////////////////////////////////////////////////////////////

    auto status = configurator->load_host_config();
    if(status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        error_exit("Failed to load host configuration from config file");
    }
    status = configurator->load_tracks();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        error_exit("Failed to load tracks from Json config file");
    }
    status = configurator->load_midi();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NO_MIDI_DEFINITIONS)
    {
        error_exit("Failed to load MIDI mapping from Json config file");
    }
    status = configurator->load_cv_gate();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NO_CV_GATE_DEFINITIONS)
    {
        error_exit("Failed to load CV and Gate configuration");
    }

    if (frontend_type == FrontendType::DUMMY || frontend_type == FrontendType::OFFLINE)
    {
        auto [status, events] = configurator->load_event_list();
        if(status == sushi::jsonconfig::JsonConfigReturnStatus::OK)
        {
            static_cast<sushi::audio_frontend::OfflineFrontend*>(audio_frontend.get())->add_sequencer_events(events);
        }
        else if (status != sushi::jsonconfig::JsonConfigReturnStatus::NO_EVENTS_DEFINITIONS)
        {
            error_exit("Failed to load Event list from Json config file");
        }
    }
    else
    {
        status = configurator->load_events();
        if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NO_EVENTS_DEFINITIONS)
        {
            error_exit("Failed to load Events from Json config file");
        }
    }
    configurator.reset();

    if (enable_parameter_dump)
    {
        std::cout << sushi::generate_processor_parameter_document(engine->controller());
        error_exit("");
    }

    if (enable_timings)
    {
        engine->performance_timer()->enable(true);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Set up Control Frontends //
    ////////////////////////////////////////////////////////////////////////////////

    if (frontend_type == FrontendType::JACK || frontend_type == FrontendType::XENOMAI_RASPA)
    {
        midi_frontend = std::make_unique<sushi::midi_frontend::AlsaMidiFrontend>(midi_inputs, midi_outputs, midi_dispatcher.get());

        osc_frontend = std::make_unique<sushi::control_frontend::OSCFrontend>(engine.get(), engine->controller(), osc_server_port, osc_send_port);
        auto osc_status = osc_frontend->init();
        if (osc_status != sushi::control_frontend::ControlFrontendStatus::OK)
        {
            error_exit("Failed to setup OSC frontend");
        }
        osc_frontend->connect_all();
    }
    else
    {
        midi_frontend = std::make_unique<sushi::midi_frontend::NullMidiFrontend>(midi_dispatcher.get());
    }

    auto midi_ok = midi_frontend->init();
    if (!midi_ok)
    {
        error_exit("Failed to setup Midi frontend");
    }
    midi_dispatcher->set_frontend(midi_frontend.get());

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    auto rpc_server = std::make_unique<sushi_rpc::GrpcServer>(grpc_listening_address, engine->controller());
#endif

    ////////////////////////////////////////////////////////////////////////////////
    // Start everything! //
    ////////////////////////////////////////////////////////////////////////////////

    audio_frontend->run();
    midi_frontend->run();

    if (frontend_type == FrontendType::JACK || frontend_type == FrontendType::XENOMAI_RASPA)
    {
        osc_frontend->run();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    SUSHI_LOG_INFO("Starting gRPC server with address: {}", grpc_listening_address);
    rpc_server->start();
#endif

    if (frontend_type != FrontendType::OFFLINE)
    {
        std::mutex m;
        std::unique_lock<std::mutex> lock(m);
        exit_notifier.wait(lock, exit_condition);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Cleanup before exiting! //
    ////////////////////////////////////////////////////////////////////////////////

    if (frontend_type == FrontendType::JACK || frontend_type == FrontendType::XENOMAI_RASPA)
    {
        osc_frontend->stop();
        midi_frontend->stop();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    rpc_server->stop();
#endif

    audio_frontend->cleanup();
    SUSHI_LOG_INFO("Sushi exited normally.");
    return 0;
}
