/**
 * @brief Offline frontend (using libsndfile) to test Sushi host and plugins
 */

#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <csignal>
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

enum class FrontendType
{
    OFFLINE,
    DUMMY,
    JACK,
    XENOMAI_RASPA,
    NONE
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
    std::cout << "SUSHI - Sensus Universal Sound Host Interface" << std::endl;
    std::cout << "Copyright 2016-2018 MIND Music Labs, Stockholm" << std::endl;
}

void print_version_and_build_info()
{
    std::cout << "\nVersion "   << SUSHI__VERSION_MAJ << "."
                                << SUSHI__VERSION_MIN << "."
                                << SUSHI__VERSION_REV << std::endl;
    std::vector<std::string> build_opts;
#ifdef SUSHI_BUILD_WITH_VST3
    build_opts.push_back("vst3");
#endif
#ifdef SUSHI_BUILD_WITH_JACK
    build_opts.push_back("jack");
#endif
#ifdef SUSHI_BUILD_WITH_XENOMAI
    build_opts.push_back("xenomai");
#endif
    std::ostringstream opts_joined;
    for (const auto& o : build_opts)
    {
        if (&o != &build_opts[0])
        {
            opts_joined << ", ";
        }
        opts_joined << o;
    }
    std::cout << "Build options enabled: " << opts_joined.str() << std::endl;

    std::cout << "Audio buffer size in frames: " << AUDIO_CHUNK_SIZE << std::endl;
#ifdef SUSHI_BUILD_WITH_XENOMAI
    std::cout << "Audio in/out channels: " << RASPA_N_CHANNELS << std::endl;
#endif
    std::cout << "Git commit: " << SUSHI_GIT_COMMIT_HASH << std::endl;
    std::cout << "Built on: " << SUSHI_BUILD_TIMESTAMP << std::endl;
}

int main(int argc, char* argv[])
{
#ifdef SUSHI_BUILD_WITH_XENOMAI
    auto ret = sushi::audio_frontend::global_init();
    if (ret < 0)
    {
        std::cerr << "Failed to initialize Xenomai process, err. code: " << ret << std::endl;
        exit(ret);
    }
#endif

    print_sushi_headline();

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
    FrontendType frontend_type = FrontendType::NONE;
    bool connect_ports = false;
    bool debug_mode_switches = false;
    int  rt_cpu_cores = 1;
    bool enable_timings = false;

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

        default:
            SushiArg::print_error("Unhandled option '", opt, "' \n");
            break;
        }
    }

    if (output_filename.empty() && !input_filename.empty())
    {
        output_filename = input_filename + "_proc.wav";
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Logger configuration
    ////////////////////////////////////////////////////////////////////////////////
    auto ret_code = MIND_LOG_SET_PARAMS(log_filename, "Logger", log_level);
    if (ret_code != MIND_LOG_ERROR_CODE_OK)
    {
        std::cerr << MIND_LOG_GET_ERROR_MESSAGE(ret_code) << ", using default." << std::endl;
    }

    MIND_GET_LOGGER_WITH_MODULE_NAME("main");

    ////////////////////////////////////////////////////////////////////////////////
    // Main body //
    ////////////////////////////////////////////////////////////////////////////////

    if (frontend_type == FrontendType::XENOMAI_RASPA)
    {
        twine::init_xenomai(); // must be called before setting up any worker pools
    }
    sushi::engine::AudioEngine engine(SUSHI_SAMPLE_RATE_DEFAULT, rt_cpu_cores);
    sushi::midi_dispatcher::MidiDispatcher midi_dispatcher(&engine);

    midi_dispatcher.set_midi_inputs(1);
    midi_dispatcher.set_midi_outputs(1);

    sushi::audio_frontend::BaseAudioFrontend* frontend;
    sushi::audio_frontend::BaseAudioFrontendConfiguration* fe_config;
    switch (frontend_type)
    {
        case FrontendType::JACK:
        {
            MIND_LOG_INFO("Setting up Jack audio frontend");
            fe_config = new sushi::audio_frontend::JackFrontendConfiguration(jack_client_name,
                                                                             jack_server_name,
                                                                             connect_ports);
            frontend = new sushi::audio_frontend::JackFrontend(&engine, &midi_dispatcher);
            break;
        }

        case FrontendType::XENOMAI_RASPA:
        {
            MIND_LOG_INFO("Setting up Xenomai RASPA frontend");
            fe_config = new sushi::audio_frontend::XenomaiRaspaFrontendConfiguration(debug_mode_switches);
            frontend = new sushi::audio_frontend::XenomaiRaspaFrontend(&engine, &midi_dispatcher);
            break;
        }

        case FrontendType::DUMMY:
        case FrontendType::OFFLINE:
        {
            bool dummy = false;
            if (frontend_type == FrontendType::DUMMY)
            {
                dummy = true;
                MIND_LOG_INFO("Setting up dummy audio frontend");
            }
            else
            {
                MIND_LOG_INFO("Setting up offline audio frontend");
            }
            fe_config = new sushi::audio_frontend::OfflineFrontendConfiguration(input_filename, output_filename, dummy);
            frontend = new sushi::audio_frontend::OfflineFrontend(&engine, &midi_dispatcher);
            break;
        }

        default:
            std::cerr << "No audio frontend selected." << std::endl;
            std::exit(1);
    }

    auto fe_ret_code = frontend->init(fe_config);
    if (fe_ret_code != sushi::audio_frontend::AudioFrontendStatus::OK)
    {
        std::cerr << "Error initializing frontend, check logs for details." << std::endl;
        std::exit(1);
    }

    sushi::jsonconfig::JsonConfigurator configurator(&engine, &midi_dispatcher);
    auto status = configurator.load_host_config(config_filename);
    if(status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        MIND_LOG_ERROR("Main: Failed to load host configuration from config file");
        std::exit(1);
    }
    status = configurator.load_tracks(config_filename);
    if(status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        MIND_LOG_ERROR("Main: Failed to load tracks from Json config file");
        std::exit(1);
    }
    configurator.load_midi(config_filename);

    if (frontend_type == FrontendType::DUMMY || frontend_type == FrontendType::OFFLINE)
    {
        auto [status, events] = configurator.load_event_list(config_filename);
        if(status == sushi::jsonconfig::JsonConfigReturnStatus::OK)
        {
            static_cast<sushi::audio_frontend::OfflineFrontend*>(frontend)->add_sequencer_events(events);
        }
    }
    else
    {
        configurator.load_events(config_filename);
    }

    if (enable_timings)
    {
        engine.enable_timing_statistics(true);
    }

    frontend->connect_control_frontends();
    frontend->run();

    if (frontend_type != FrontendType::OFFLINE)
    {
        std::mutex m;
        std::unique_lock<std::mutex> lock(m);
        exit_notifier.wait(lock, exit_condition);
    }

    frontend->cleanup();
    MIND_LOG_INFO("Sushi exited normally.");
    return 0;
}
