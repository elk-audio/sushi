# SUSHI
## Sensus Universal Sound Host Interface

Headless plugin host for ELK music operating system.

## Usage

See `sushi -h` for a complete list of options.
Common use cases are:

  * Test in offline mode with I/O from audio file:

    sushi -c config_file.json input_file.wav

  * Use JACK for realtime audio:

    sushi -j -c config_file.json

With JACK, sushi creates 8 virtual input and output ports that you can connect to other programs or system outputs.

## Configuration file examples

See directory `example_configs` for the JSON-schema definition and some example configurations.
Configuration files are used for global host configs, track and plugins configuration, MIDI routing and mapping, events sequencing.

## Run-time OSC control

Sushi listen on port 24024 for the following OSC commands:

Path                                   | Typespec | Arguments
---------------------------------------|----------|--------------------------------------
/parameter/plugin_id/parameter_name    |  f       | parameter value
/keyboard/track_name                   |  sif     | event type, note index, norm. value
/engine/add_track                      |  si      | name, n. of channels
/engine/delete_track                   |  s       | name
/engine/add_processor                  |  sssss   | track, id, name, file path, type
/engine/delete_processor               |  ss      | track name, plugin id


Copyright 2016-2018 MIND Music Labs AB, Stockholm, Sweden.
