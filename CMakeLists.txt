cmake_minimum_required(VERSION 3.20)

#####################
#  Project Version  #
#####################

# Don't change version anywhere else. Everything is generated from this.
set(SUSHI_VERSION_MAJOR 0)
set(SUSHI_VERSION_MINOR 12)
set(SUSHI_VERSION_REVISION 0)

project(sushi
    VERSION ${SUSHI_VERSION_MAJOR}.${SUSHI_VERSION_MINOR}.${SUSHI_VERSION_REVISION}
    DESCRIPTION "Headless plugin host for Elk Audio OS"
    HOMEPAGE_URL "https://github.com/elk-audio/sushi"
    LANGUAGES CXX
)

##################################
#  Perform Cross Compile setup   #
##################################

# Procedure is performed only if cross compile is detected, i.e CMAKE_SYSROOT
# has been explicitly defined
# if(NOT "$ENV{CMAKE_SYSROOT}" STREQUAL "")
#     set(CMAKE_SYSROOT "$ENV{CMAKE_SYSROOT}")
#     set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
#     set(CMAKE_PROGRAM_PATH "$ENV{CMAKE_PROGRAM_PATH}")
#     set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
#     set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#     set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# endif()

##################################
#  Generate build information    #
##################################

# Get the latest commit hash of the working branch
execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
)

###########################
#  Build and link options #
###########################

# Default behaviour is to build and link with everything
option(WITH_XENOMAI "Enable Xenomai support" ON)
option(WITH_JACK "Enable Jack support" ON)
option(WITH_PORTAUDIO "Enable PortAudio support" ON)
option(WITH_ALSA_MIDI "Enable alsa midi support" ON)
option(WITH_RT_MIDI "Enable RtMidi support" OFF)
option(WITH_VST2 "Enable Vst 2 support" ON)
option(WITH_VST3 "Enable Vst 3 support" ON)
option(WITH_LV2 "Enable LV 2 support" ON)
option(WITH_LV2_MDA_TESTS "Include unit tests depending on LV2 drobilla MDA plugin port." ON)
option(WITH_UNIT_TESTS "Build and run unit tests after compilation" ON)
option(WITH_LINK "Enable Ableton Link support" ON)
option(WITH_RPC_INTERFACE "Enable RPC control support" ON)
option(BUILD_TWINE "Build included Twine library" ON)

if (WITH_ALSA_MIDI AND WITH_RT_MIDI)
    message(FATAL_ERROR "Both alsa midi and RtMidi set to on. Use only one midi frontend at a time")
endif()

set(AUDIO_BUFFER_SIZE 64 CACHE STRING "Set internal audio buffer size in frames")

###############################
# Sub-directory configuration #
###############################

if (${BUILD_TWINE})
    set(TWINE_WITH_TESTS OFF CACHE BOOL "")
    add_subdirectory(twine)
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} twine)
    set(TWINE_LIB twine)
else()
    find_path(TWINE_PATH NAMES "twine/twine.h")
    find_library(TWINE_LIB
            NAMES twine
            PATHS /usr/lib /usr/local/lib
    )
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${TWINE_PATH})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${twine_lib})
endif()

add_subdirectory(third-party EXCLUDE_FROM_ALL)

#################
# Xenomai setup #
#################

if (${WITH_XENOMAI})
    message("Building with Xenomai support")

    # External libraries:
    # explicitly link with cobalt lib
    set(XENOMAI_BASE_DIR "/usr/xenomai" CACHE STRING "xenomai base dir path")

    if(NOT "$ENV{CMAKE_SYSROOT}" STREQUAL "")
        set(CMAKE_SYSROOT "$ENV{CMAKE_SYSROOT}")
        message("ENV_CMAKE_SYSROOT = " $ENV{CMAKE_SYSROOT})
    endif()
    if(NOT "${CMAKE_SYSROOT}" STREQUAL "")
        set(XENOMAI_BASE_DIR "${CMAKE_SYSROOT}/usr/xenomai")
        message("XENOMAI_BASE_DIR is " ${XENOMAI_BASE_DIR})
    endif()

    find_library(COBALT_LIB cobalt HINTS ${XENOMAI_BASE_DIR}/lib)

    set(TWINE_WITH_XENOMAI ON CACHE BOOL "" FORCE)
    find_library(RASPA_LIB
                 NAMES raspa
                 PATHS /usr/lib /usr/local/lib
                 )
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} asound ${RASPA_LIB} ${COBALT_LIB})

    # Compile definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_XENOMAI)
else()
    set(TWINE_WITH_XENOMAI OFF CACHE BOOL "" FORCE)
endif()

##############
# Jack setup #
##############

if (${WITH_JACK})
    message("Building with Jack support.")

    # Linked libraries
    find_path(jack_dir NAMES "jack/jack.h")
    find_library(jack_lib NAMES jack)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${jack_dir})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${jack_lib})

    # Compile definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_JACK)
endif()

###################
# PortAudio setup #
###################

if (${WITH_PORTAUDIO})
    message("Building with PortAudio support.")

    # Linked libraries
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/third-party/portaudio/include) # For working tests since we need the headers but don't want to link with the actual library.
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} PortAudio)

    # Compile definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_PORTAUDIO)
endif()

###################
# Alsa midi setup #
###################

if (${WITH_ALSA_MIDI})
    message("Building with alsa midi support.")

    # Additional sources
    set(MIDI_SOURCES src/control_frontends/alsa_midi_frontend.h src/control_frontends/alsa_midi_frontend.cpp)

    # linked libraries
    find_path(ALSA_DIR NAMES "alsa/asoundlib.h")
    find_library(ALSA_LIB NAMES asound)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${ALSA_DIR})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${ALSA_LIB})

    # Compile definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_ALSA_MIDI)
endif()

#################
# RtMidi ssetup #
#################

if (${WITH_RT_MIDI})
    message("Building with RtMidi support.")

    # Additional sources
    set(MIDI_SOURCES ${MIDI_SOURCES} src/control_frontends/rt_midi_frontend.cpp src/control_frontends/rt_midi_frontend.h)

    # Linked Libraries
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} rtmidi)

    # Compile definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_RT_MIDI)
endif()

###############
# VST 2 setup #
###############

if (${WITH_VST2})
    message("Building with Vst2 support.")

    set(ADDITIONAL_VST2_SOURCES src/library/vst2x/vst2x_wrapper.h
                                src/library/vst2x/vst2x_host_callback.h
                                src/library/vst2x/vst2x_plugin_loader.h
                                src/library/vst2x/vst2x_midi_event_fifo.h
                                src/library/vst2x/vst2x_processor_factory.h
                                src/library/vst2x/vst2x_wrapper.cpp
                                src/library/vst2x/vst2x_host_callback.cpp
                                src/library/vst2x/vst2x_plugin_loader.cpp)

    # Include dirs
    if (VST_2_SDK_BASE_DIR)
        set(VST2_SDK_PATH ${VST_2_SDK_BASE_DIR})
    else()
        find_file(VST2_SDK_PATH NAMES VST2_SDK vstsdk2.4 HINTS $ENV{HOME}/workspace
                                                                $ENV{HOME}/workspace/VST_SDK
                                                                $ENV{HOME}/workspace/vstsdk3610_11_06_2018_build_37)
    endif()
    if (${VST2_SDK_PATH} STREQUAL "VST2_SDK_PATH-NOTFOUND")
        message("Warning! Could not find Steinberg SDK for VST 2.4")
    endif()

    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${VST2_SDK_PATH}/pluginterfaces/vst2.x)

    # Compile definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_VST2 -D__cdecl=)
endif()

###############
# VST 3 setup #
###############

if (${WITH_VST3})
    message("Building with Vst3 support.")

    # Vst 3 host:
    # Build the given Vst3 host implementation into a separate library to avoid
    # Vst specific defines leaking into our code.

    set(VST3_SDK_PATH "${PROJECT_SOURCE_DIR}/third-party/vst3sdk")
    set(VST3_HOST_SOURCES
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/eventlist.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/parameterchanges.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/hostclasses.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/utility/stringconvert.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/pluginterfacesupport.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/common/memorystream.cpp")

    set(VST3_COMPILE_FLAGS "-Wno-unused-parameter -Wno-extra")

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
            set(VST3_HOST_SOURCES ${VST3_HOST_SOURCES} "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module_linux.cpp")
            set(AUDIOHOST_PLATFORM_LIBS)
    endif()

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            set(VST3_HOST_SOURCES
                ${VST3_HOST_SOURCES}
                "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module_mac.mm")
            set(AUDIOHOST_PLATFORM_LIBS "-framework Cocoa")
            set(VST3_COMPILE_FLAGS "${VST3_COMPILE_FLAGS} -fobjc-arc")
    endif()

    add_library(vst3_host STATIC ${VST3_HOST_SOURCES})
    target_link_libraries(vst3_host PRIVATE ${AUDIOHOST_PLATFORM_LIBS})
    set_target_properties(vst3_host PROPERTIES EXCLUDE_FROM_ALL true)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set_target_properties(vst3_host PROPERTIES COMPILE_FLAGS "-DRELEASE ${VST3_COMPILE_FLAGS}")
    else()
        set_target_properties(vst3_host PROPERTIES COMPILE_FLAGS "-DDEVELOPMENT ${VST3_COMPILE_FLAGS}")
    endif()

    target_include_directories(vst3_host PRIVATE "${PROJECT_SOURCE_DIR}/third-party/vst3sdk")
    add_dependencies(vst3_host sdk)

    # Set sources
    set(ADDITIONAL_VST3_SOURCES src/library/vst3x/vst3x_wrapper.h
                                src/library/vst3x/vst3x_host_app.h
                                src/library/vst3x/vst3x_utils.h
                                src/library/vst3x/vst3x_processor_factory.h
                                src/library/vst3x/vst3x_wrapper.cpp
                                src/library/vst3x/vst3x_host_app.cpp
                                src/library/vst3x/vst3x_utils.cpp)

    # Linked libraries
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} vst3_host sdk)

    # Compiler definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_VST3)
endif()

#############
# LV2 setup #
#############

if (${WITH_LV2})
    message("Building with LV2 support.")

    set(ADDITIONAL_LV2_SOURCES src/library/lv2/lv2_wrapper.h
                               src/library/lv2/lv2_control.h
                               src/library/lv2/lv2_features.h
                               src/library/lv2/lv2_host_nodes.h
                               src/library/lv2/lv2_model.h
                               src/library/lv2/lv2_port.h
                               src/library/lv2/lv2_state.h
                               src/library/lv2/lv2_processor_factory.h
                               src/library/lv2/lv2_wrapper.cpp
                               src/library/lv2/lv2_state.cpp
                               src/library/lv2/lv2_worker.cpp
                               src/library/lv2/lv2_features.cpp
                               src/library/lv2/lv2_model.cpp
                               src/library/lv2/lv2_port.cpp
                               src/library/lv2/lv2_control.cpp)

    # Linked libraries
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} lilv-0 lv2_host)

    # Compiler definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_LV2)
endif()

##############
# gRPC setup #
##############

if (${WITH_RPC_INTERFACE})
    message("Building with RPC support.")

    add_subdirectory(rpc_interface)

    # Linked libraries
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} sushi_rpc)

    # Compiler definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_RPC_INTERFACE)
endif()

######################
# Ableton link setup #
######################

if (${WITH_LINK})
    message("Building with Ableton Link support.")

    include(third-party/link/AbletonLinkConfig.cmake)

    # Linked libraries
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} Ableton::Link)

    # Compiler definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_ABLETON_LINK)
endif()

message("Configured audio buffer size: " ${AUDIO_BUFFER_SIZE} " samples")

####################
#  Main Target     #
####################

set(COMPILATION_UNITS src/main.cpp
                      src/logging.cpp
                      src/audio_frontends/base_audio_frontend.cpp
                      src/audio_frontends/offline_frontend.cpp
                      src/audio_frontends/jack_frontend.cpp
                      src/audio_frontends/portaudio_frontend.cpp
                      src/audio_frontends/xenomai_raspa_frontend.cpp
                      src/control_frontends/base_control_frontend.cpp
                      src/control_frontends/oscpack_osc_messenger.cpp
                      src/control_frontends/osc_frontend.cpp
                      src/dsp_library/biquad_filter.cpp
                      src/engine/audio_engine.cpp
                      src/engine/audio_graph.cpp
                      src/engine/event_dispatcher.cpp
                      src/engine/track.cpp
                      src/engine/midi_dispatcher.cpp
                      src/engine/json_configurator.cpp
                      src/engine/receiver.cpp
                      src/engine/event_timer.cpp
                      src/engine/transport.cpp
                      src/engine/parameter_manager.cpp
                      src/engine/processor_container.cpp
                      src/engine/controller/controller.cpp
                      src/engine/controller/system_controller.cpp
                      src/engine/controller/transport_controller.cpp
                      src/engine/controller/timing_controller.cpp
                      src/engine/controller/keyboard_controller.cpp
                      src/engine/controller/audio_graph_controller.cpp
                      src/engine/controller/parameter_controller.cpp
                      src/engine/controller/program_controller.cpp
                      src/engine/controller/midi_controller.cpp
                      src/engine/controller/audio_routing_controller.cpp
                      src/engine/controller/cv_gate_controller.cpp
                      src/engine/controller/osc_controller.cpp
                      src/engine/controller/session_controller.cpp
                      src/library/event.cpp
                      src/library/midi_decoder.cpp
                      src/library/midi_encoder.cpp
                      src/library/internal_plugin.cpp
                      src/library/performance_timer.cpp
                      src/library/parameter_dump.cpp
                      src/library/processor.cpp
                      src/library/processor_state.cpp
                      src/library/plugin_registry.cpp
                      src/library/internal_processor_factory.cpp
                      src/library/lv2/lv2_processor_factory.cpp
                      src/library/vst2x/vst2x_processor_factory.cpp
                      src/library/vst3x/vst3x_processor_factory.cpp
                      src/plugins/arpeggiator_plugin.cpp
                      src/plugins/control_to_cv_plugin.cpp
                      src/plugins/cv_to_control_plugin.cpp
                      src/plugins/gain_plugin.cpp
                      src/plugins/lfo_plugin.cpp
                      src/plugins/passthrough_plugin.cpp
                      src/plugins/equalizer_plugin.cpp
                      src/plugins/peak_meter_plugin.cpp
                      src/plugins/return_plugin.cpp
                      src/plugins/sample_player_plugin.cpp
                      src/plugins/sample_player_voice.cpp
                      src/plugins/sample_delay_plugin.cpp
                      src/plugins/send_plugin.cpp
                      src/plugins/send_return_factory.cpp
                      src/plugins/step_sequencer_plugin.cpp
                      src/plugins/transposer_plugin.cpp
                      src/plugins/wav_writer_plugin.cpp
                      src/plugins/mono_summing_plugin.cpp
                      src/plugins/stereo_mixer_plugin.cpp
                      src/audio_frontends/offline_frontend.cpp
        )

# Enumerate all the headers separately so that CLion can index them
set(EXTRA_CLION_SOURCES src/logging.h
                        src/options.h
                        src/compile_time_settings.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/audio_frontend_internals.h
                        src/audio_frontends/offline_frontend.h
                        src/audio_frontends/jack_frontend.h
                        src/audio_frontends/portaudio_frontend.h
                        src/audio_frontends/xenomai_raspa_frontend.h
                        src/control_frontends/base_control_frontend.h
                        src/control_frontends/osc_frontend.h
                        src/control_frontends/osc_utils.h
                        src/control_frontends/oscpack_osc_messenger.h
                        src/control_frontends/base_midi_frontend.h
                        src/dsp_library/envelopes.h
                        src/dsp_library/sample_wrapper.h
                        src/dsp_library/biquad_filter.h
                        src/dsp_library/value_smoother.h
                        src/library/base_performance_timer.h
                        src/library/event.h
                        src/library/event_interface.h
                        src/library/sample_buffer.h
                        src/library/midi_decoder.h
                        src/library/midi_encoder.h
                        src/library/rt_event.h
                        src/library/processor.h
                        src/library/base_processor_factory.h
                        src/library/plugin_registry.h
                        src/library/internal_processor_factory.h
                        src/library/performance_timer.h
                        src/library/internal_plugin.h
                        src/library/rt_event_fifo.h
                        src/library/rt_event_pipe.h
                        src/library/spinlock.h
                        src/library/simple_fifo.h
                        src/library/synchronised_fifo.h
                        src/library/time.h
                        src/engine/base_engine.h
                        src/engine/base_processor_container.h
                        src/engine/audio_engine.h
                        src/engine/audio_graph.h
                        src/engine/track.h
                        src/engine/receiver.h
                        src/engine/midi_dispatcher.h
                        src/engine/event_dispatcher.h
                        src/engine/base_event_dispatcher.h
                        src/engine/event_timer.h
                        src/engine/json_configurator.h
                        src/engine/midi_receiver.h
                        src/engine/host_control.h
                        src/engine/transport.h
                        src/engine/parameter_manager.h
                        src/engine/processor_container.h
                        src/engine/controller/controller.h
                        src/engine/controller/system_controller.h
                        src/engine/controller/transport_controller.h
                        src/engine/controller/timing_controller.h
                        src/engine/controller/keyboard_controller.h
                        src/engine/controller/audio_graph_controller.h
                        src/engine/controller/parameter_controller.h
                        src/engine/controller/program_controller.h
                        src/engine/controller/midi_controller.h
                        src/engine/controller/audio_routing_controller.h
                        src/engine/controller/cv_gate_controller.h
                        src/engine/controller/osc_controller.h
                        src/plugins/arpeggiator_plugin.h
                        src/plugins/control_to_cv_plugin.h
                        src/plugins/cv_to_control_plugin.h
                        src/plugins/gain_plugin.h
                        src/plugins/lfo_plugin.h
                        src/plugins/passthrough_plugin.h
                        src/plugins/equalizer_plugin.h
                        src/plugins/peak_meter_plugin.h
                        src/plugins/return_plugin.h
                        src/plugins/transposer_plugin.h
                        src/plugins/sample_player_plugin.h
                        src/plugins/sample_player_voice.h
                        src/plugins/sample_delay_plugin.h
                        src/plugins/send_plugin.h
                        src/plugins/send_return_factory.h
                        src/plugins/step_sequencer_plugin.h
                        src/plugins/wav_writer_plugin.h
                        src/plugins/mono_summing_plugin.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/offline_frontend.h
        )

set(SOURCE_FILES "${COMPILATION_UNITS}" "${EXTRA_CLION_SOURCES}")


add_executable(sushi MACOSX_BUNDLE "${COMPILATION_UNITS}"
                     "${EXTRA_CLION_SOURCES}"
                     "${ADDITIONAL_VST2_SOURCES}"
                     "${ADDITIONAL_VST3_SOURCES}"
                     "${ADDITIONAL_LV2_SOURCES}"
                     "${MIDI_SOURCES}")

#########################
#  Include Directories  #
#########################

set(INCLUDE_DIRS ${INCLUDE_DIRS}
                 "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include"
                 "${CMAKE_BINARY_DIR}" # for generated version.h
                 "${PROJECT_SOURCE_DIR}"
                 "${PROJECT_SOURCE_DIR}/third-party/optionparser/"
                 "${PROJECT_SOURCE_DIR}/third-party/rapidjson/include"
                 "${PROJECT_SOURCE_DIR}/third-party/vst3sdk"
                 "${PROJECT_SOURCE_DIR}/third-party/link/include")

#################################
#  Linked libraries             #
#################################

find_package(SndFile CONFIG REQUIRED)

set(INCLUDE_DIRS
    ${INCLUDE_DIRS}
)

set(COMMON_LIBRARIES
    SndFile::sndfile
    pthread
    dl
    fifo
    spdlog::spdlog
    oscpack
    ${TWINE_LIB}
)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(COMMON_LIBRARIES ${COMMON_LIBRARIES} atomic)
endif()

target_include_directories(sushi PRIVATE ${INCLUDE_DIRS})
target_link_libraries(sushi PRIVATE ${EXTRA_BUILD_LIBRARIES} ${COMMON_LIBRARIES})

####################################
#  Compiler Flags and definitions  #
####################################

target_compile_features(sushi PRIVATE cxx_std_17)
target_compile_options(sushi PRIVATE -Wall -Wextra -Wno-psabi -fno-rtti -ffast-math)
target_compile_definitions(sushi PRIVATE -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=${AUDIO_BUFFER_SIZE} ${EXTRA_COMPILE_DEFINITIONS})

######################
#  Tests subproject  #
######################

if (${WITH_UNIT_TESTS})
    add_subdirectory(test)
endif()

####################
#  Install         #
####################
set(DOC_FILES_INSTALL
    "${PROJECT_SOURCE_DIR}/LICENSE.md"
    "${PROJECT_SOURCE_DIR}/README.md"
    "${PROJECT_SOURCE_DIR}/HISTORY.md"
)

if (APPLE)
    install(CODE [[
    include(BundleUtilities)
    fixup_bundle($<TARGET_FILE:sushi> "" "") # TODO: fix to work when not submodule
    ]] COMPONENT Runtime)
endif()

install(TARGETS sushi RUNTIME DESTINATION bin, BUNDLE DESTINATION Applications)
foreach(ITEM ${DOC_FILES_INSTALL})
    install(FILES ${ITEM} DESTINATION share/sushi/doc)
endforeach()
