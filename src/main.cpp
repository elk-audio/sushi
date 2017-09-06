/**
 * @brief Offline frontend (using libsndfile) to test Sushi host and plugins
 */

#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

#include "logging.h"
#include "options.h"
#include "version.h"
#include "audio_frontends/offline_frontend.h"
#include "audio_frontends/jack_frontend.h"
#include "engine/json_configurator.h"
#include "audio_frontends/xenomai_frontend.h"

void print_sushi_headline()
{
    std::cout << "SUSHI - Sensus Universal Sound Host Interface" << std::endl;
    std::cout << "Copyright 2016-2017 MIND Music Labs, Stockholm" << std::endl;
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
    #ifdef __COBALT__
        build_opts.push_back("xenomai_cobalt");
    #elif defined __MERCURY__
        build_opts.push_back("xenomai_mercury");
    #endif
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
    std::cout << "Git commit: " << SUSHI_GIT_COMMIT_HASH << std::endl;
    std::cout << "Built on: " << SUSHI_BUILD_TIMESTAMP << std::endl;
}

int main(int argc, char* argv[])
{
    print_sushi_headline();
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
    if ( (cl_parser.nonOptionsCount() == 0 && cl_parser.optionsCount() == 0) || (cl_options[OPT_IDX_HELP]) )
    {
        optionparser::printUsage(fwrite, stdout, usage);
        return 0;
    }

    std::string input_filename;
    std::string output_filename;
    if (cl_parser.nonOptionsCount() > 0)
    {
        input_filename = std::string(cl_parser.nonOption(0));
        // By default, prepend sushiproc_ to filename
        output_filename = std::string(input_filename);
        output_filename.append(".proc.wav");
    }

    std::string log_level = std::string(SUSHI_LOG_LEVEL_DEFAULT);
    std::string log_filename = std::string(SUSHI_LOG_FILENAME_DEFAULT);
    std::string config_filename = std::string(SUSHI_JSON_FILENAME_DEFAULT);
    std::string jack_client_name = std::string(SUSHI_JACK_CLIENT_NAME_DEFAULT);
    std::string jack_server_name = std::string("");
    bool use_jack = false;
    bool use_xenomai = false;
    bool connect_ports = false;

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

        case OPT_IDX_OUTPUT_FILE:
            output_filename.assign(opt.arg);
            break;

        case OPT_IDX_USE_JACK:
            use_jack = true;
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

        case OPT_IDX_USE_XENOMAI:
            use_xenomai = true;
            break;

        default:
            SushiArg::print_error("Unhandled option '", opt, "' \n");
            break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Logger configuration
    ////////////////////////////////////////////////////////////////////////////////
    auto ret_code = MIND_LOG_SET_PARAMS(log_filename, "Logger", log_level);
    if (ret_code != MIND_LOG_ERROR_CODE_OK)
    {
        std::cerr << MIND_LOG_GET_ERROR_MESSAGE(ret_code) << ", using default." << std::endl;
    }

    MIND_GET_LOGGER;

    ////////////////////////////////////////////////////////////////////////////////
    // Main body //
    ////////////////////////////////////////////////////////////////////////////////
    sushi::engine::AudioEngine engine(SUSHI_SAMPLE_RATE_DEFAULT);
    sushi::midi_dispatcher::MidiDispatcher midi_dispatcher(&engine);
    engine.set_audio_input_channels(2);
    engine.set_audio_output_channels(2);
    midi_dispatcher.set_midi_input_ports(1);

    sushi::jsonconfig::JsonConfigurator configurator(&engine, &midi_dispatcher);
    auto status = configurator.load_host_config(config_filename);
    if(status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        MIND_LOG_ERROR("Main: Failed to load host configuration from config file");
        std::exit(1);
    }
    status = configurator.load_chains(config_filename);
    if(status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        MIND_LOG_ERROR("Main: Failed to load chains from Json config file");
        std::exit(1);
    }
   configurator.load_midi(config_filename);

    sushi::audio_frontend::BaseAudioFrontend* frontend;
    sushi::audio_frontend::BaseAudioFrontendConfiguration* fe_config;
    if (use_jack)
    {
        MIND_LOG_INFO("Setting up Jack audio frontend");
        fe_config = new sushi::audio_frontend::JackFrontendConfiguration(jack_client_name,
                                                                         jack_server_name,
                                                                         connect_ports);
        frontend = new sushi::audio_frontend::JackFrontend(&engine, &midi_dispatcher);
    }
    else if (use_xenomai)
    {
        MIND_LOG_INFO("Setting up Xenomai offline audio frontend");
#ifdef SUSHI_BUILD_WITH_XENOMAI
        xenomai_init(&argc, const_cast<char* const**>(&argv));
#endif
        fe_config = new sushi::audio_frontend::XenomaiFrontendConfiguration(input_filename, output_filename);
        frontend = new sushi::audio_frontend::XenomaiFrontend(&engine, &midi_dispatcher);
    }
    else
    {
        MIND_LOG_INFO("Setting up offline audio frontend");
        fe_config = new sushi::audio_frontend::OfflineFrontendConfiguration(input_filename, output_filename);
        frontend = new sushi::audio_frontend::OfflineFrontend(&engine, &midi_dispatcher);
    }

    auto fe_ret_code = frontend->init(fe_config);
    if (fe_ret_code != sushi::audio_frontend::AudioFrontendStatus::OK)
    {
        std::cerr << "Error initializing frontend, check logs for details." << std::endl;
        std::exit(1);
    }
    if (!use_jack && !use_xenomai)
    {
        rapidjson::Document config;
        status = configurator.parse_events_from_file(config_filename, config);
        if(status == sushi::jsonconfig::JsonConfigReturnStatus::OK)
        {
            static_cast<sushi::audio_frontend::OfflineFrontend*>(frontend)->add_sequencer_events_from_json_def(config);
        }
    }
    else if (use_xenomai)
    {
        rapidjson::Document config;
        status = configurator.parse_events_from_file(config_filename, config);
        if(status == sushi::jsonconfig::JsonConfigReturnStatus::OK)
        {
            static_cast<sushi::audio_frontend::XenomaiFrontend*>(frontend)->add_sequencer_events_from_json_def(config);
        }
    }
    frontend->run();

    frontend->cleanup();
    return 0;
}