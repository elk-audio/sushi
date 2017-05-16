/**
 * @brief Offline frontend (using libsndfile) to test Sushi host and plugins
 */

#include <vector>
#include <fstream>
#include <iostream>
#include <cstdlib>

#include <json/json.h>

#include "logging.h"
#include "options.h"
#include "audio_frontends/offline_frontend.h"
#include "audio_frontends/jack_frontend.h"
#include "library/json_configurer.h"


int main(int argc, char* argv[])
{
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

    option::Stats cl_stats(usage, argc, argv);
    std::vector<option::Option> cl_options(cl_stats.options_max);
    std::vector<option::Option> cl_buffer(cl_stats.buffer_max);
    option::Parser cl_parser(usage, argc, argv, &cl_options[0], &cl_buffer[0]);

    if (cl_parser.error())
    {
        return 1;
    }
    if ( (cl_parser.nonOptionsCount() == 0 && cl_parser.optionsCount() == 0) || (cl_options[OPT_IDX_HELP]) )
    {
        option::printUsage(fwrite, stdout, usage);
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
    bool connect_ports = false;

    for (int i=0; i<cl_parser.optionsCount(); i++)
    {
        option::Option& opt = cl_buffer[i];
        switch(opt.index())
        {
        case OPT_IDX_HELP:
        case OPT_IDX_UNKNOWN:
            // should be handled before arriving here
            assert(false);
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
        fprintf(stderr, "%s, using default.\n", MIND_LOG_GET_ERROR_MESSAGE(ret_code).c_str());
    }

    MIND_GET_LOGGER;

    // JSON configuration parsing

    MIND_LOG_INFO("Reading configuration file {}", config_filename);
    std::ifstream file(config_filename);
    if (!file.good())
    {
        MIND_LOG_ERROR("Couldn't open JSON configuration file: {}", config_filename);
        std::exit(1);
    }

    Json::Value config;
    Json::Reader reader;


    

    bool parse_ok = reader.parse(file, config, false);
    if (!parse_ok)
    {
        MIND_LOG_ERROR("Error parsing JSON configuration file, {}", reader.getFormattedErrorMessages());
        //std::exit(1);
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Main body //
    ////////////////////////////////////////////////////////////////////////////////
    sushi::engine::AudioEngine engine(SUSHI_SAMPLE_RATE_DEFAULT);
    engine.set_audio_input_channels(2);
    engine.set_audio_output_channels(2);
    engine.set_midi_input_ports(1);

    /*===========================================================
    =            Using Jsonconfigurer to init chains            =
    ===========================================================*/
    sushi::jsonconfigurer::JsonConfigurer testconfig;
    sushi::jsonconfigurer::JsonConfigReturnStatus jsonstatus;
    jsonstatus = testconfig.init_configurer(&engine, config_filename);
    if(jsonstatus != sushi::jsonconfigurer::JsonConfigReturnStatus::OK)
    {
        std::exit(1);
    }
    jsonstatus = testconfig.init_chains_from_jsonconfig();
    if(jsonstatus != sushi::jsonconfigurer::JsonConfigReturnStatus::OK)
    {
        std::exit(1);
    } 
    /*=====  End of Using Jsonconfigurer to init chains  ======*/
    
    engine.init_midi_from_json_array(config["midi"]);

    sushi::audio_frontend::BaseAudioFrontend* frontend;
    sushi::audio_frontend::BaseAudioFrontendConfiguration* fe_config;
    if (use_jack)
    {
        fe_config = new sushi::audio_frontend::JackFrontendConfiguration(jack_client_name,
                                                                         jack_server_name,
                                                                         connect_ports);
        frontend = new sushi::audio_frontend::JackFrontend(&engine);
    }
    else
    {
        fe_config = new sushi::audio_frontend::OfflineFrontendConfiguration(input_filename, output_filename);
        frontend = new sushi::audio_frontend::OfflineFrontend(&engine);
    }

    auto fe_ret_code = frontend->init(fe_config);
    if (fe_ret_code != sushi::audio_frontend::AudioFrontendStatus::OK)
    {
        fprintf(stderr, "Error initializing frontend, check logs for details.\n");
        std::exit(1);
    }
    if (!use_jack)
    {
        fe_ret_code = static_cast<sushi::audio_frontend::OfflineFrontend*>(frontend)->add_sequencer_events_from_json_def(config["events"]);
        if (fe_ret_code != sushi::audio_frontend::AudioFrontendStatus::OK)
        {
            fprintf(stderr, "Error initializing sequencer events from JSON, check logs for details.\n");
            std::exit(1);
        }
    }
    frontend->run();

    frontend->cleanup();
    return 0;
}

