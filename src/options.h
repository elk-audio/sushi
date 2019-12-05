/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Option parsing
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */
#include <cstdio>
#include "optionparser.h"

#define SUSHI_Q(x) #x
#define SUSHI_QUOTE(x) SUSHI_Q(x)

////////////////////////////////////////////////////////////////////////////////
// Options Defaults
////////////////////////////////////////////////////////////////////////////////

#define SUSHI_LOG_LEVEL_DEFAULT "info"
#define SUSHI_LOG_FILENAME_DEFAULT "/tmp/sushi.log"
#define SUSHI_JSON_FILENAME_DEFAULT "config.json"
#define SUSHI_SAMPLE_RATE_DEFAULT 48000
#define SUSHI_JACK_CLIENT_NAME_DEFAULT "sushi"
#define SUSHI_OSC_SERVER_PORT 24024
#define SUSHI_OSC_SEND_PORT 24023
#define SUSHI_GRPC_LISTENING_PORT "[::]:51051"

////////////////////////////////////////////////////////////////////////////////
// Helpers for optionparse
////////////////////////////////////////////////////////////////////////////////

struct SushiArg : public optionparser::Arg
{

    static void print_error(const char* msg1, const optionparser::Option& opt, const char* msg2)
    {
        fprintf(stderr, "%s", msg1);
        fwrite(opt.name, static_cast<size_t>(opt.namelen), 1, stderr);
        fprintf(stderr, "%s", msg2);
    }

    static optionparser::ArgStatus Unknown(const optionparser::Option& option, bool msg)
    {
        if (msg)
        {
            print_error("Unknown option '", option, "'\n");
        }
        return optionparser::ARG_ILLEGAL;
    }

    static optionparser::ArgStatus NonEmpty(const optionparser::Option& option, bool msg)
    {
        if (option.arg != 0 && option.arg[0] != 0)
        {
            return optionparser::ARG_OK;
        }

        if (msg)
        {
            print_error("Option '", option, "' requires a non-empty argument\n");
        }
        return optionparser::ARG_ILLEGAL;
    }

    static optionparser::ArgStatus Numeric(const optionparser::Option& option, bool msg)
    {
        char* endptr = 0;
        if (option.arg != 0 && strtol(option.arg, &endptr, 10))
        {};

        if (endptr != option.arg && *endptr == 0)
        {
          return optionparser::ARG_OK;
        }
        if (msg)
        {
            print_error("Option '", option, "' requires a numeric argument\n");
        }
        return optionparser::ARG_ILLEGAL;
    }

};

////////////////////////////////////////////////////////////////////////////////
// Command Line options descriptors
////////////////////////////////////////////////////////////////////////////////

// List here the different command line options
enum OptionIndex
{ 
    OPT_IDX_UNKNOWN,
    OPT_IDX_HELP,
    OPT_IDX_VERSION,
    OPT_IDX_LOG_LEVEL,
    OPT_IDX_LOG_FILE,
    OPT_IDX_LOG_FLUSH_INTERVAL,
    OPT_IDX_CONFIG_FILE,
    OPT_IDX_USE_OFFLINE,
    OPT_IDX_INPUT_FILE,
    OPT_IDX_OUTPUT_FILE,
    OPT_IDX_USE_DUMMY,
    OPT_IDX_USE_JACK,
    OPT_IDX_CONNECT_PORTS,
    OPT_IDX_JACK_CLIENT,
    OPT_IDX_JACK_SERVER,
    OPT_IDX_USE_XENOMAI_RASPA,
    OPT_IDX_XENOMAI_DEBUG_MODE_SW,
    OPT_IDX_MULTICORE_PROCESSING,
    OPT_IDX_TIMINGS_STATISTICS,
    OPT_IDX_OSC_RECEIVE_PORT,
    OPT_IDX_OSC_SEND_PORT,
    OPT_IDX_GRPC_LISTEN_ADDRESS
};

// Option types (UNUSED is generally used for options that take a value as argument)
enum OptionType
{
    OPT_TYPE_UNUSED = 0,
    OPT_TYPE_DISABLED = 1,
    OPT_TYPE_ENABLED = 2
};

// Option descriptors, one for each entry of the OptionIndex enum
const optionparser::Descriptor usage[] =
{
    {
        OPT_IDX_UNKNOWN,    // index
        OPT_TYPE_UNUSED,    // type
        "",         // shortopt
        "",         // longopt
        SushiArg::Unknown, // check_arg
        "\nUSAGE: sushi -r|-j|-o|-d [options] \n\nOptions:" // help
    },
    {
        OPT_IDX_HELP,
        OPT_TYPE_UNUSED,
        "h?",
        "help",
        SushiArg::None,
        "\t\t-h --help \tPrint usage and exit."
    },
    {
        OPT_IDX_VERSION,
        OPT_TYPE_UNUSED,
        "v",
        "version",
        SushiArg::None,
        "\t\t-v --version \tPrint version information and exit."
    },
    {
        OPT_IDX_LOG_LEVEL,
        OPT_TYPE_UNUSED,
        "l",
        "log-level",
        SushiArg::NonEmpty,
        "\t\t-l <level>, --log-level=<level> \tSpecify minimum logging level, from ('debug', 'info', 'warning', 'error') [default=" SUSHI_LOG_LEVEL_DEFAULT "]."
    },
    {
        OPT_IDX_LOG_FILE,
        OPT_TYPE_UNUSED,
        "L",
        "log-file",
        SushiArg::NonEmpty,
        "\t\t-L <filename>, --log-file=<filename> \tSpecify logging file destination [default=" SUSHI_LOG_FILENAME_DEFAULT "]."
    },
    {
        OPT_IDX_LOG_FLUSH_INTERVAL,
        OPT_TYPE_UNUSED,
        "",
        "log-flush-interval",
        SushiArg::NonEmpty,
        "\t\t --log-flush-interval=<seconds> \tEnable flushing the log periodically and specify the interval."
    },
    {
        OPT_IDX_CONFIG_FILE,
        OPT_TYPE_UNUSED,
        "c",
        "config-file",
        SushiArg::NonEmpty,
        "\t\t-c <filename>, --config-file=<filename> \tSpecify configuration JSON file [default=" SUSHI_JSON_FILENAME_DEFAULT "]."
    },
    {
        OPT_IDX_USE_OFFLINE,
        OPT_TYPE_DISABLED,
        "o",
        "offline",
        SushiArg::Optional,
        "\t\t-o --offline \tUse offline file audio frontend."
    },
    {
        OPT_IDX_INPUT_FILE,
        OPT_TYPE_UNUSED,
        "i",
        "input",
        SushiArg::NonEmpty,
        "\t\t-i <filename>, --input=<filename> \tSpecify input file, required for --offline option."
    },
    {
        OPT_IDX_OUTPUT_FILE,
        OPT_TYPE_UNUSED,
        "O",
        "output",
        SushiArg::NonEmpty,
        "\t\t-O <filename>, --output=<filename> \tSpecify output file [default= (input_file).proc.wav]."
    },
    {
        OPT_IDX_USE_DUMMY,
        OPT_TYPE_DISABLED,
        "d",
        "dummy",
        SushiArg::Optional,
        "\t\t-d --dummy \tUse dummy audio frontend. Useful for debugging."
    },
    {
        OPT_IDX_USE_JACK,
        OPT_TYPE_DISABLED,
        "j",
        "jack",
        SushiArg::Optional,
        "\t\t-j --jack \tUse Jack realtime audio frontend."
    },
    {
        OPT_IDX_CONNECT_PORTS,
        OPT_TYPE_DISABLED,
        "",
        "connect-ports",
        SushiArg::Optional,
        "\t\t--connect-ports \tTry to automatically connect Jack ports at startup."
    },
    {
        OPT_IDX_JACK_CLIENT,
        OPT_TYPE_UNUSED,
        "",
        "client-name",
        SushiArg::NonEmpty,
        "\t\t--client-name=<jack client name> \tSpecify name of Jack client [default=sushi]."
    },
    {
        OPT_IDX_JACK_SERVER,
        OPT_TYPE_UNUSED,
        "",
        "server-name",
        SushiArg::NonEmpty,
        "\t\t--server-name=<jack server name> \tSpecify name of Jack server to connect to [determined by jack if empty]."
    },
    {
        OPT_IDX_USE_XENOMAI_RASPA,
        OPT_TYPE_DISABLED,
        "r",
        "raspa",
        SushiArg::Optional,
        "\t\t-r --raspa \tUse Xenomai real-time frontend with RASPA driver."
    },
    {
        OPT_IDX_XENOMAI_DEBUG_MODE_SW,
        OPT_TYPE_DISABLED,
        "",
        "debug-mode-sw",
        SushiArg::Optional,
        "\t\t--debug-mode-sw \tBreak to debugger if a mode switch is detected (Xenomai only)."
    },
    {
        OPT_IDX_MULTICORE_PROCESSING,
        OPT_TYPE_UNUSED,
        "m",
        "multicore-processing",
        SushiArg::Numeric,
        "\t\t-m <n>, --multicore-processing=<n> \tProcess audio multithreaded with n cores [default n=1 (off)]."
    },
    {
        OPT_IDX_TIMINGS_STATISTICS,
        OPT_TYPE_DISABLED,
        "",
        "timing-statistics",
        SushiArg::Optional,
        "\t\t--timing-statistics \tEnable performance timings on all audio processors."
    },
    {
        OPT_IDX_OSC_RECEIVE_PORT,
        OPT_TYPE_UNUSED,
        "p",
        "osc-rcv-port",
        SushiArg::NonEmpty,
        "\t\t-p <port> --osc-rcv-port=<port> \tPort to listen for OSC messages on [default port=" SUSHI_QUOTE(SUSHI_OSC_SERVER_PORT) "]."
    },
    {
        OPT_IDX_OSC_SEND_PORT,
        OPT_TYPE_UNUSED,
        "",
        "osc-send-port",
        SushiArg::NonEmpty,
        "\t\t--osc-send-port=<port> \tPort to output OSC messages to [default port=" SUSHI_QUOTE(SUSHI_OSC_SEND_PORT) "]."
    },
    {
        OPT_IDX_GRPC_LISTEN_ADDRESS,
        OPT_TYPE_UNUSED,
        "",
        "grpc-address",
        SushiArg::NonEmpty,
        "\t\t--grpc-address=<port> \tgRPC listening address in the format: address:port. By default accepts incoming connections from all ip:s [default port=" SUSHI_GRPC_LISTENING_PORT "]."
    },
    // Don't touch this one (set default values for optionparse library)
    { 0, 0, 0, 0, 0, 0}
};
