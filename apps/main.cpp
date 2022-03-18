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
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <vector>
#include <iostream>
#include <csignal>
#include <optional>
#include <condition_variable>
#include <filesystem>

#include "twine/src/twine_internal.h"

#include "logging.h"
#include "engine/audio_engine.h"
#include "audio_frontends/offline_frontend.h"
#include "audio_frontends/jack_frontend.h"
#include "audio_frontends/xenomai_raspa_frontend.h"
#include "audio_frontends/portaudio_frontend.h"
#include "audio_frontends/portaudio_devices_dump.h"
#include "engine/json_configurator.h"
#include "control_frontends/osc_frontend.h"
#include "control_frontends/oscpack_osc_messenger.h"

#include "library/parameter_dump.h"
#include "compile_time_settings.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
#include "control_frontends/alsa_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RT_MIDI
#include "control_frontends/rt_midi_frontend.h"
#endif

enum class FrontendType
{
    OFFLINE,
    DUMMY,
    JACK,
    PORTAUDIO,
    XENOMAI_RASPA,
    NONE
};

bool                    exit_flag = false;
bool                    exit_condition() {return exit_flag;}
std::condition_variable exit_notifier;

void signal_handler([[maybe_unused]] int sig)
{
    exit_flag = true;
    exit_notifier.notify_one();
}

void print_sushi_headline()
{
    std::cout << "SUSHI - Copyright 2017-2022 Elk, Stockholm" << std::endl;
    std::cout << "SUSHI is licensed under the Affero GPL 3.0. Source code is available at github.com/elk-audio" << std::endl;
}

void error_exit(const std::string& message)
{
    std::cerr << message << std::endl;
    std::exit(1);
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

struct SushiOptions
{
    std::string input_filename;
    std::string output_filename;

    std::string log_level = std::string(SUSHI_LOG_LEVEL_DEFAULT);
    std::string log_filename = std::string(SUSHI_LOG_FILENAME_DEFAULT);
    std::string config_filename = std::string(SUSHI_JSON_FILENAME_DEFAULT);
    std::string jack_client_name = std::string(SUSHI_JACK_CLIENT_NAME_DEFAULT);
    std::string jack_server_name = std::string("");
    int osc_server_port = SUSHI_OSC_SERVER_PORT_DEFAULT;
    int osc_send_port = SUSHI_OSC_SEND_PORT_DEFAULT;
    auto osc_send_ip = SUSHI_OSC_SEND_IP_DEFAULT;

    std::optional<int> portaudio_input_device_id = std::nullopt;
    std::optional<int> portaudio_output_device_id = std::nullopt;
    float portaudio_suggested_input_latency = SUSHI_PORTAUDIO_INPUT_LATENCY_DEFAULT;
    float portaudio_suggested_output_latency = SUSHI_PORTAUDIO_OUTPUT_LATENCY_DEFAULT;
    bool enable_portaudio_devs_dump = false;
    std::string grpc_listening_address = SUSHI_GRPC_LISTENING_PORT_DEFAULT;
    FrontendType frontend_type = FrontendType::NONE;
    bool connect_ports = false;
    bool debug_mode_switches = false;
    int  rt_cpu_cores = 1;
    bool enable_timings = false;
    bool enable_flush_interval = false;
    bool enable_parameter_dump = false;
    bool use_osc = true;
    bool use_grpc = true;
    std::chrono::seconds log_flush_interval = std::chrono::seconds(0);
    std::string base_plugin_path = std::filesystem::current_path();
    std::string sentry_crash_handler_path = SUSHI_SENTRY_CRASH_HANDLER_PATH_DEFAULT;
    std::string sentry_dsn = SUSHI_SENTRY_DSN_DEFAULT;
};

class SushiModel
{
    public:
    std::unique_ptr<sushi::engine::AudioEngine> engine {nullptr};

    std::unique_ptr<sushi::midi_frontend::BaseMidiFrontend> midi_frontend {nullptr};
    std::unique_ptr<sushi::control_frontend::OSCFrontend> osc_frontend {nullptr};
    std::unique_ptr<sushi::audio_frontend::BaseAudioFrontend> audio_frontend {nullptr};
    std::unique_ptr<sushi::audio_frontend::BaseAudioFrontendConfiguration> frontend_config {nullptr};

    std::unique_ptr<sushi::midi_dispatcher::MidiDispatcher> midi_dispatcher {nullptr};

    std::unique_ptr<sushi::jsonconfig::JsonConfigurator> configurator {nullptr};

    std::unique_ptr<sushi::engine::Controller> controller {nullptr};

    std::unique_ptr<sushi_rpc::GrpcServer> rpc_server {nullptr};
};

enum class PARSE_STATUS
{
    OK,
    ERROR,
    MISSING_ARGUMENTS,
    EXIT
};

PARSE_STATUS parse_options(int argc, char* argv[], SushiOptions& options)
{
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
        return PARSE_STATUS::ERROR;
    }

    if (cl_parser.optionsCount() == 0 || cl_options[OPT_IDX_HELP])
    {
        optionparser::printUsage(fwrite, stdout, usage);
        return PARSE_STATUS::MISSING_ARGUMENTS;
    }

    for (int i = 0; i < cl_parser.optionsCount(); i++)
    {
        optionparser::Option& opt = cl_buffer[i];
        switch (opt.index())
        {
            case OPT_IDX_HELP:
            case OPT_IDX_UNKNOWN:
                // should be handled before arriving here
                assert(false);
                break;

            case OPT_IDX_VERSION:
                {
                    print_version_and_build_info();

                    // TODO: If this is a plugin it makes no sense to exit.
                    //  Add a check / assertion that this doesn't happen when embedded?
                    return PARSE_STATUS::EXIT;
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

        case OPT_IDX_PA_SUGGESTED_INPUT_LATENCY:
            portaudio_suggested_input_latency = atof(opt.arg);
            break;

        case OPT_IDX_PA_SUGGESTED_OUTPUT_LATENCY:
            portaudio_suggested_output_latency = atof(opt.arg);
            break;

        case OPT_IDX_DUMP_PORTAUDIO:
            enable_portaudio_devs_dump = true;
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

        case OPT_IDX_OSC_SEND_IP:
            osc_send_ip = opt.arg;
            break;

            case OPT_IDX_GRPC_LISTEN_ADDRESS:
                options.grpc_listening_address = opt.arg;
                break;

            case OPT_IDX_NO_OSC:
            use_osc = false;
            break;

        case OPT_IDX_NO_GRPC:
            use_grpc = false;
            break;

        case OPT_IDX_BASE_PLUGIN_PATH:
            base_plugin_path = std::string(opt.arg);
            break;

        case OPT_IDX_SENTRY_CRASH_HANDLER:
            sentry_crash_handler_path = opt.arg;
            break;

        case OPT_IDX_SENTRY_DSN:
            sentry_dsn = opt.arg;
            break;default:
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

    return PARSE_STATUS::OK;
}

void init_logger(SushiOptions& options)
{
    auto ret_code = SUSHI_INITIALIZE_LOGGER(options.log_filename,
                                            "Logger",
                                            options.log_level,
                                            options.enable_flush_interval,
                                            options.log_flush_interval,
                                            sentry_crash_handler_path, sentry_dsn);
    if (ret_code != SUSHI_LOG_ERROR_CODE_OK)
    {
        std::cerr << SUSHI_LOG_GET_ERROR_MESSAGE(ret_code) << ", using default." << std::endl;
    }
}

enum class INIT_STATUS
{
    OK,
    FAILED_LOAD_HOST_CONFIG, // "Failed to load host configuration from config file"
    FAILED_LOAD_TRACKS, // "Failed to load tracks from Json config file"
    FAILED_LOAD_MIDI_MAPPING, // "Failed to load MIDI mapping from Json config file"
    FAILED_LOAD_CV_GATE, // "Failed to load CV and Gate configuration"
    FAILED_LOAD_PROCESSOR_STATES, // "Failed to load initial processor states"
    FAILED_LOAD_EVENT_LIST, // "Failed to load Event list from Json config file";
    FAILED_LOAD_EVENTS      //  "Failed to load Events from Json config file"
};

INIT_STATUS load_configuration(SushiModel& model, SushiOptions& options)
{
    auto status = model.configurator->load_host_config();
    if(status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        return INIT_STATUS::FAILED_LOAD_HOST_CONFIG;
    }

    status = model.configurator->load_tracks();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        return INIT_STATUS::FAILED_LOAD_TRACKS;
    }

    status = model.configurator->load_midi();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return INIT_STATUS::FAILED_LOAD_MIDI_MAPPING;
    }

    status = model.configurator->load_cv_gate();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return INIT_STATUS::FAILED_LOAD_CV_GATE;
    }

    status = model.configurator->load_initial_state();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return INIT_STATUS::FAILED_LOAD_PROCESSOR_STATES;
    }

    if (options.frontend_type == FrontendType::DUMMY ||
        options.frontend_type == FrontendType::OFFLINE)
    {
        auto [status, events] = model.configurator->load_event_list();
        if(status == sushi::jsonconfig::JsonConfigReturnStatus::OK)
        {
            static_cast<sushi::audio_frontend::OfflineFrontend*>(model.audio_frontend.get())->add_sequencer_events(events);
        }
        else if (status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            return INIT_STATUS::FAILED_LOAD_EVENT_LIST;
        }
    }
    else
    {
        status = model.configurator->load_events();
        if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            return INIT_STATUS::FAILED_LOAD_EVENTS;
        }
    }

    return INIT_STATUS::OK;
}

// TODO: Re-consider return type.
std::tuple<SushiModel, INIT_STATUS> init(SushiOptions& options)
{
    SUSHI_GET_LOGGER_WITH_MODULE_NAME("main");

#ifdef SUSHI_BUILD_WITH_XENOMAI
    auto ret = sushi::audio_frontend::XenomaiRaspaFrontend::global_init();
    if (ret < 0)
    {
        error_exit("Failed to initialize Xenomai process, err. code: " + std::to_string(ret));
    }
#endif

    SushiModel model;

    if (enable_portaudio_devs_dump)
    {
#ifdef SUSHI_BUILD_WITH_PORTAUDIO
        std::cout << sushi::audio_frontend::generate_portaudio_devices_info_document() << std::endl;
        std::exit(0);
#else
        std::cerr << "SUSHI not built with Portaudio support, cannot dump devices." << std::endl;
#endif
    }

    if (options.frontend_type == FrontendType::XENOMAI_RASPA)
    {
        twine::init_xenomai(); // must be called before setting up any worker pools
    }

    model.engine = std::make_unique<sushi::engine::AudioEngine>(SUSHI_SAMPLE_RATE_DEFAULT,
                                                                options.rt_cpu_cores,
                                                                options.debug_mode_switches,
                                                                nullptr);
    if (! base_plugin_path.empty())
    {
        engine->set_base_plugin_path(base_plugin_path);
    }

    model.midi_dispatcher = std::make_unique<sushi::midi_dispatcher::MidiDispatcher>(model.engine->event_dispatcher());

    model.configurator = std::make_unique<sushi::jsonconfig::JsonConfigurator>(model.engine.get(),
                                                                               model.midi_dispatcher.get(),
                                                                               model.engine->processor_container(),
                                                                               options.config_filename);

    auto [audio_config_status, audio_config] = model.configurator->load_audio_config();

    if (audio_config_status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        if (audio_config_status == sushi::jsonconfig::JsonConfigReturnStatus::INVALID_FILE)
        {
            // TODO: Return INIT_STATUS
            error_exit("Error reading config file, invalid file path: " + options.config_filename);
        }

        // TODO: Return INIT_STATUS
        error_exit("Error reading host config, check logs for details.");
    }

    auto status = configurator->load_host_config();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        error_exit("Failed to load host configuration from config file");
    }

    int cv_inputs = audio_config.cv_inputs.value_or(0);
    int cv_outputs = audio_config.cv_outputs.value_or(0);
    int midi_inputs = audio_config.midi_inputs.value_or(1);
    int midi_outputs = audio_config.midi_outputs.value_or(1);

#ifdef SUSHI_BUILD_WITH_RT_MIDI
    auto rt_midi_input_mappings = audio_config.rt_midi_input_mappings;
    auto rt_midi_output_mappings = audio_config.rt_midi_output_mappings;
#endif

    model.midi_dispatcher->set_midi_inputs(midi_inputs);
    model.midi_dispatcher->set_midi_outputs(midi_outputs);

    ////////////////////////////////////////////////////////////////////////////////
    // Set up Audio Frontend //
    ////////////////////////////////////////////////////////////////////////////////

    switch (options.frontend_type)
    {
        case FrontendType::JACK:
        {
            SUSHI_LOG_INFO("Setting up Jack audio frontend");
            model.frontend_config = std::make_unique<sushi::audio_frontend::JackFrontendConfiguration>(options.jack_client_name,
                                                                                                       options.jack_server_name,
                                                                                                       options.connect_ports,
                                                                                                       cv_inputs,
                                                                                                       cv_outputs);

            model.audio_frontend = std::make_unique<sushi::audio_frontend::JackFrontend>(model.engine.get());
            break;
        }

        case FrontendType::PORTAUDIO:
        {
            SUSHI_LOG_INFO("Setting up PortAudio frontend");
            model.frontend_config = std::make_unique<sushi::audio_frontend::PortAudioFrontendConfiguration>(options.portaudio_input_device_id,
                                                                                                            options.portaudio_output_device_id,
                                                                                                            portaudio_suggested_input_latency,
                                                                                                      portaudio_suggested_output_latency,cv_inputs,
                                                                                                            cv_outputs);
            model.audio_frontend = std::make_unique<sushi::audio_frontend::PortAudioFrontend>(model.engine.get());
            break;
        }

        case FrontendType::XENOMAI_RASPA:
        {
            SUSHI_LOG_INFO("Setting up Xenomai RASPA frontend");
            model.frontend_config = std::make_unique<sushi::audio_frontend::XenomaiRaspaFrontendConfiguration>(options.debug_mode_switches,
                                                                                                               cv_inputs,
                                                                                                               cv_outputs);
            model.audio_frontend = std::make_unique<sushi::audio_frontend::XenomaiRaspaFrontend>(model.engine.get());
            break;
        }

        case FrontendType::DUMMY:
        case FrontendType::OFFLINE:
        {
            bool dummy = false;
            if (options.frontend_type == FrontendType::DUMMY)
            {
                dummy = true;
                SUSHI_LOG_INFO("Setting up dummy audio frontend");
            }
            else
            {
                SUSHI_LOG_INFO("Setting up offline audio frontend");
            }
            model.frontend_config = std::make_unique<sushi::audio_frontend::OfflineFrontendConfiguration>(options.input_filename,
                                                                                                          options.output_filename,
                                                                                                          dummy,
                                                                                                          cv_inputs,
                                                                                                          cv_outputs);
            model.audio_frontend = std::make_unique<sushi::audio_frontend::OfflineFrontend>(model.engine.get());
            break;
        }

        default:
        {
            // TODO: Return INIT_STATUS
            error_exit("No audio frontend selected.");
        }
    }

    auto audio_frontend_status = model.audio_frontend->init(model.frontend_config.get());
    if (audio_frontend_status != sushi::audio_frontend::AudioFrontendStatus::OK)
    {
        // TODO: Return INIT_STATUS
        error_exit("Error initializing frontend, check logs for details.");
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Load Configuration //
    ////////////////////////////////////////////////////////////////////////////////

    auto status = load_configuration(model, options);
    if (status != INIT_STATUS::OK)
    {
        return {std::move(model), status};
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Set up Controller and Control Frontends //
    ////////////////////////////////////////////////////////////////////////////////
    model.controller = std::make_unique<sushi::engine::Controller>(model.engine.get(),
                                                                  model.midi_dispatcher.get(),
                                                                  audio_frontend.get());

    // TODO: Parameter dump functionality should not be here!
    if (options.enable_parameter_dump)
    {
        std::cout << sushi::generate_processor_parameter_document(model.controller.get());
        // TODO: Return INIT_STATUS
        std::exit(0);
    }

    if (options.frontend_type == FrontendType::JACK ||
        options.frontend_type == FrontendType::XENOMAI_RASPA ||
        options.frontend_type == FrontendType::PORTAUDIO)
    {
#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
        model.midi_frontend = std::make_unique<sushi::midi_frontend::AlsaMidiFrontend>(midi_inputs,
                                                                                       midi_outputs,
                                                                                       model.midi_dispatcher.get());
#elif SUSHI_BUILD_WITH_RT_MIDI
        model.midi_frontend = std::make_unique<sushi::midi_frontend::RtMidiFrontend>(midi_inputs,
                                                                                     midi_outputs,
                                                                                     std::move(rt_midi_input_mappings),
                                                                                     std::move(rt_midi_output_mappings),
                                                                                     model.midi_dispatcher.get());
#else
        midi_frontend = std::make_unique<sushi::midi_frontend::NullMidiFrontend>(midi_inputs,
                                                                                 midi_outputs,
                                                                                 model.midi_dispatcher.get());
#endif
        if (use_osc)
        {
            SUSHI_LOG_INFO("Listening to OSC messages on port {}. Transmitting to port {}, on IP {}.", osc_server_port, osc_send_port, osc_send_ip);
            auto oscpack_messenger = new sushi::osc::OscpackOscMessenger(osc_server_port, osc_send_port, osc_send_ip);
            model.osc_frontend = std::make_unique<sushi::control_frontend::OSCFrontend>(model.engine.get(),
                                                                                  model.controller.get(),
                                                                                  options.osc_server_port,
                                                                                  options.osc_send_port);
            model.controller->set_osc_frontend(model.osc_frontend.get());
            model.configurator->set_osc_frontend(model.osc_frontend.get());

            auto osc_status = model.osc_frontend->init();
            if (osc_status != sushi::control_frontend::ControlFrontendStatus::OK)
            {
            // TODO: Return INIT_STATUS
                error_exit("Failed to setup OSC frontend");
            }

            auto status = model.configurator->load_osc();
        if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            // TODO: Return INIT_STATUS
                error_exit("Failed to load OSC echo specification from Json config file");
            }
        }
    }
    else
    {
        model.midi_frontend = std::make_unique<sushi::midi_frontend::NullMidiFrontend>(model.midi_dispatcher.get());
    }

    model.configurator.reset();

    auto midi_ok = model.midi_frontend->init();
    if (!midi_ok)
    {
        error_exit("Failed to setup Midi frontend");
    }
    model.midi_dispatcher->set_frontend(model.midi_frontend.get());

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    if (use_grpc)
    {
        model.rpc_server = std::make_unique<sushi_rpc::GrpcServer>(options.grpc_listening_address, controller.get());
    }
#endif

    return {std::move(model), INIT_STATUS::OK};
}

void start(SushiModel& model, SushiOptions& options)
{
    SUSHI_GET_LOGGER_WITH_MODULE_NAME("main");

    if (options.enable_timings)
    {
        model.engine->performance_timer()->enable(true);
    }

    model.audio_frontend->run();
    model.engine->event_dispatcher()->run();
    model.midi_frontend->run();

    if (use_osc && (options.frontend_type == FrontendType::JACK ||
                    options.frontend_type == FrontendType::XENOMAI_RASPA ||
                    options.frontend_type == FrontendType::PORTAUDIO))
    {
        model.osc_frontend->run();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    if (use_grpc)
    {
        SUSHI_LOG_INFO("Starting gRPC server with address: {}", options.grpc_listening_address);
        model.rpc_server->start();
    }
#endif
}

void exit(SushiModel& model, SushiOptions& options)
{
    model.audio_frontend->cleanup();
    model.engine->event_dispatcher()->stop();

    if (osc_frontend && (options.frontend_type == FrontendType::JACK ||
                         options.frontend_type == FrontendType::XENOMAI_RASPA ||
                         options.frontend_type == FrontendType::PORTAUDIO))
    {
        model.osc_frontend->stop();
        model.midi_frontend->stop();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    if (rpc_server)
    {
        model.rpc_server->stop();
    }
#endif
}

void act_on_init_status(INIT_STATUS init_status)
{
    switch (init_status)
    {
        case INIT_STATUS::FAILED_LOAD_HOST_CONFIG:
            error_exit("Failed to load host configuration from config file"); break;
        case INIT_STATUS::FAILED_LOAD_TRACKS:
            error_exit("Failed to load tracks from Json config file"); break;
        case INIT_STATUS::FAILED_LOAD_MIDI_MAPPING:
            error_exit("Failed to load MIDI mapping from Json config file"); break;
        case INIT_STATUS::FAILED_LOAD_CV_GATE:
            error_exit("Failed to load CV and Gate configuration"); break;
        case INIT_STATUS::FAILED_LOAD_PROCESSOR_STATES:
            error_exit("Failed to load initial processor states"); break;
        case INIT_STATUS::FAILED_LOAD_EVENT_LIST:
            error_exit("Failed to load Event list from Json config file"); break;
        case INIT_STATUS::FAILED_LOAD_EVENTS:
            error_exit("Failed to load Events from Json config file"); break;
        case INIT_STATUS::OK:
            break;
    };
}

int main(int argc, char* argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    SushiOptions options;

    auto option_status = parse_options(argc, argv, options);
    if (option_status == PARSE_STATUS::ERROR)
    {
        return 1;
    }
    else if (option_status == PARSE_STATUS::MISSING_ARGUMENTS)
    {
        return 2;
    }
    else if (option_status == PARSE_STATUS::EXIT)
    {
        return 0;
    }

    // TODO: Parameter dump and exit makes no sense outside of standalone Sushi!
    //  Design and implement alternative solution. Future story?
    if (options.enable_parameter_dump == false)
    {
        print_sushi_headline();
    }

    init_logger(options);

    // TODO: move initialization into constructor, or still have init(...) pattern?

    auto [model, init_status] = init(options);

    act_on_init_status(init_status);

    assert(init_status == INIT_STATUS::OK);

    start(model, options);

    if (options.frontend_type != FrontendType::OFFLINE)
    {
        std::mutex m;
        std::unique_lock<std::mutex> lock(m);
        exit_notifier.wait(lock, exit_condition);
    }

    exit(model, options);

    SUSHI_GET_LOGGER_WITH_MODULE_NAME("main");
    SUSHI_LOG_INFO("Sushi exiting normally!");

    return 0;
}
