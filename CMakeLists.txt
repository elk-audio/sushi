cmake_minimum_required(VERSION 3.8)
project(sushi)

##################################
#  Perform Cross Compile setup   #
##################################

# Procedure is performed only if cross compile is detected, i.e CMAKE_SYSROOT
# has been explicitly defined
if(NOT "$ENV{CMAKE_SYSROOT}" STREQUAL "")
    set(CMAKE_SYSROOT "$ENV{CMAKE_SYSROOT}")
    set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

##################################
#  Generate build information    #
##################################

# Get the latest commit hash of the working branch
execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M")

configure_file(
    ${CMAKE_SOURCE_DIR}/include/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
)

###########################
#  Build and link options #
###########################

# Default behaviour is to build and link with everything
option(WITH_XENOMAI "Enable Xenomai support" ON)
option(WITH_JACK "Enable Jack support" ON)
option(WITH_VST2 "Enable Vst 2 support" ON)
option(WITH_VST3 "Enable Vst 3 support" ON)
option(WITH_UNIT_TESTS "Build and run unit tests after compilation" ON)
option(WITH_LINK "Enable Ableton Link support" ON)
option(BUILD_TWINE "Build included Twine library" ON)
option(WITH_RPC_INTERFACE "Enable RPC control support" ON)

set(AUDIO_BUFFER_SIZE 64 CACHE STRING "Set internal audio buffer size in frames")

if (${WITH_XENOMAI})
    message("Building with Xenomai support")
endif()
if (${WITH_JACK})
    message("Building with Jack support.")
endif()
if (${WITH_VST2})
    message("Building with Vst2 support.")
endif()
if (${WITH_VST3})
    message("Building with Vst3 support.")
endif()
if (${WITH_RPC_INTERFACE})
    message("Building with RPC support.")
endif()
if (${WITH_LINK})
    message("Building with Ableton Link support.")
endif()

message("Configured audio buffer size: " ${AUDIO_BUFFER_SIZE} " samples")


#############
# Vst3 Host #
#############

if (${WITH_VST3})

# Build the given Vst3 host implementation into a separate library to avoid
# Vst specific defines leaking into our code.

set(VST3_SDK_PATH "${PROJECT_SOURCE_DIR}/third-party/vst3sdk")
set(VST3_HOST_SOURCES
        "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/eventlist.cpp"
        "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/parameterchanges.cpp"
        "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/hostclasses.cpp"
        "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module_linux.cpp"
        "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module.cpp"
        "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/stringconvert.cpp"
        "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/pluginterfacesupport.cpp"
        "${VST3_SDK_PATH}/public.sdk/source/common/memorystream.cpp")

add_library(vst3_host STATIC ${VST3_HOST_SOURCES})
set_target_properties(vst3_host PROPERTIES EXCLUDE_FROM_ALL true)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set_target_properties(vst3_host PROPERTIES COMPILE_FLAGS "-DRELEASE -Wno-unused-parameter -Wno-extra")
else()
    set_target_properties(vst3_host PROPERTIES COMPILE_FLAGS "-DDEVELOPMENT -Wno-unused-parameter -Wno-extra")
endif()
target_include_directories(vst3_host PRIVATE "${PROJECT_SOURCE_DIR}/third-party/vst3sdk")
add_dependencies(vst3_host sdk)
endif()

####################
#  Main Target     #
####################

set(COMPILATION_UNITS src/main.cpp
                      src/logging.cpp
                      src/audio_frontends/offline_frontend.cpp
                      src/audio_frontends/jack_frontend.cpp
                      src/audio_frontends/xenomai_raspa_frontend.cpp
                      src/control_frontends/base_control_frontend.cpp
                      src/control_frontends/osc_frontend.cpp
                      src/dsp_library/biquad_filter.cpp
                      src/engine/audio_engine.cpp
                      src/engine/controller.cpp
                      src/engine/event_dispatcher.cpp
                      src/engine/track.cpp
                      src/engine/midi_dispatcher.cpp
                      src/engine/json_configurator.cpp
                      src/engine/receiver.cpp
                      src/engine/event_timer.cpp
                      src/engine/transport.cpp
                      src/library/event.cpp
                      src/library/midi_decoder.cpp
                      src/library/midi_encoder.cpp
                      src/library/internal_plugin.cpp
                      src/library/performance_timer.cpp
                      src/library/parameter_dump.cpp
                      src/library/processor.cpp
                      src/library/vst2x_wrapper.cpp
                      src/library/vst3x_wrapper.cpp
                      src/plugins/arpeggiator_plugin.cpp
                      src/plugins/control_to_cv_plugin.cpp
                      src/plugins/cv_to_control_plugin.cpp
                      src/plugins/gain_plugin.cpp
                      src/plugins/lfo_plugin.cpp
                      src/plugins/passthrough_plugin.cpp
                      src/plugins/equalizer_plugin.cpp
                      src/plugins/peak_meter_plugin.cpp
                      src/plugins/transposer_plugin.cpp
                      src/plugins/sample_player_plugin.cpp
                      src/plugins/sample_player_voice.cpp
                      src/plugins/step_sequencer_plugin.cpp
                      src/audio_frontends/offline_frontend.cpp
        )

# Enumerate all the headers separately so that CLion can index them
set(EXTRA_CLION_SOURCES src/logging.h
                        src/options.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/audio_frontend_internals.h
                        src/audio_frontends/offline_frontend.h
                        src/audio_frontends/jack_frontend.h
                        src/audio_frontends/xenomai_raspa_frontend.h
                        src/control_frontends/base_control_frontend.h
                        src/control_frontends/osc_frontend.h
                        src/control_frontends/osc_utils.h
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
                        src/library/performance_timer.h
                        src/library/internal_plugin.h
                        src/library/rt_event_fifo.h
                        src/library/rt_event_pipe.h
                        src/library/spinlock.h
                        src/library/simple_fifo.h
                        src/library/synchronised_fifo.h
                        src/library/time.h
                        src/library/vst2x_wrapper.h
                        src/library/vst3x_wrapper.h
                        src/engine/base_engine.h
                        src/engine/audio_engine.h
                        src/engine/controller.h
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
                        src/plugins/arpeggiator_plugin.h
                        src/plugins/control_to_cv_plugin.h
                        src/plugins/cv_to_control_plugin.h
                        src/plugins/gain_plugin.h
                        src/plugins/lfo_plugin.h
                        src/plugins/passthrough_plugin.h
                        src/plugins/equalizer_plugin.h
                        src/plugins/peak_meter_plugin.h
                        src/plugins/transposer_plugin.h
                        src/plugins/sample_player_plugin.h
                        src/plugins/sample_player_voice.h
                        src/plugins/step_sequencer_plugin.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/offline_frontend.h
        )

set(SOURCE_FILES "${COMPILATION_UNITS}" "${EXTRA_CLION_SOURCES}")

if (${WITH_XENOMAI} OR ${WITH_JACK})
    set(ADDITIONAL_ALSA_SOURCES src/control_frontends/alsa_midi_frontend.h
                                src/control_frontends/alsa_midi_frontend.cpp)
endif()

if (${WITH_VST2})
    set(ADDITIONAL_VST2_SOURCES src/library/vst2x_host_callback.cpp
                                src/library/vst2x_host_callback.h
                                src/library/vst2x_plugin_loader.cpp
                                src/library/vst2x_plugin_loader.h
                                src/library/vst2x_midi_event_fifo.h)
endif()

if (${WITH_VST3})
    set(ADDITIONAL_VST3_SOURCES src/library/vst3x_host_app.cpp
                                src/library/vst3x_utils.cpp
                                src/library/vst3x_host_app.h
                                src/library/vst3x_utils.h)
endif()

add_executable(sushi "${COMPILATION_UNITS}"
                     "${EXTRA_CLION_SOURCES}"
                     "${ADDITIONAL_VST2_SOURCES}"
                     "${ADDITIONAL_VST3_SOURCES}"
                     "${ADDITIONAL_ALSA_SOURCES}")

#########################
#  Include Directories  #
#########################

if (${WITH_VST2})
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
endif()

set(INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include"
                 "${CMAKE_BINARY_DIR}" # for generated version.h
                 "${PROJECT_SOURCE_DIR}"
                 "${PROJECT_SOURCE_DIR}/third-party/optionparser/"
                 "${PROJECT_SOURCE_DIR}/third-party/rapidjson/include"
                 "${PROJECT_SOURCE_DIR}/third-party/spdlog/include"
                 "${PROJECT_SOURCE_DIR}/third-party/vst3sdk"
                 "${PROJECT_SOURCE_DIR}/third-party/link/include")

# /usr/local doesn't get added by default in Mac OS X
if (APPLE)
    set(INCLUDE_DIRS "${INCLUDE_DIRS}" /usr/local/include)
endif()

################################
#  Subdirectory configuration  #
################################


if (${WITH_XENOMAI})
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
else()
    set(TWINE_WITH_XENOMAI OFF CACHE BOOL "" FORCE)
endif()

if (${BUILD_TWINE})
    set(TWINE_WITH_TESTS OFF CACHE BOOL "")
    add_subdirectory(twine)
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} twine)
    set(TWINE_LIB twine)
else()
    find_library(TWINE_LIB
            NAMES twine
            PATHS /usr/lib /usr/local/lib
    )
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${twine_lib})
endif()

if (${WITH_LINK})
    include(third-party/link/AbletonLinkConfig.cmake)
endif()

if (${WITH_RPC_INTERFACE})
    add_subdirectory(rpc_interface)
endif()

add_subdirectory(third-party EXCLUDE_FROM_ALL)

#################################
#  Linked libraries             #
#################################

# Common libraries between all builds, and safe to be included in test targets
set(COMMON_LIBRARIES
    sndfile
    lo
    pthread
    dl
    fifo
    ${TWINE_LIB}
)

if (${WITH_VST3})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} vst3_host sdk)
endif()

if (WITH_VST2)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${VST2_SDK_PATH}/pluginterfaces/vst2.x)
endif()

if (${WITH_JACK})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} jack asound)
endif()

if (${WITH_LINK})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} Ableton::Link)
endif()

if (${WITH_RPC_INTERFACE})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} sushi_rpc)
endif()

target_include_directories(sushi PRIVATE ${INCLUDE_DIRS})
target_link_libraries(sushi PRIVATE ${EXTRA_BUILD_LIBRARIES} ${COMMON_LIBRARIES})

####################################
#  Compiler Flags and definitions  #
####################################

target_compile_features(sushi PRIVATE cxx_std_17)
target_compile_options(sushi PRIVATE -Wall -Wextra -Wno-psabi -fno-rtti -ffast-math)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0"))
        target_compile_options(sushi PRIVATE -faligned-new)
    endif()
endif()

target_compile_definitions(sushi PRIVATE -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=${AUDIO_BUFFER_SIZE})

if (${WITH_XENOMAI})
    target_compile_definitions(sushi PRIVATE -DSUSHI_BUILD_WITH_XENOMAI)
endif()

if (${WITH_JACK})
    target_compile_definitions(sushi PRIVATE -DSUSHI_BUILD_WITH_JACK)
endif()

if (${WITH_VST3})
    target_compile_definitions(sushi PRIVATE -DSUSHI_BUILD_WITH_VST3)
endif()

if (${WITH_LINK})
    target_compile_definitions(sushi PRIVATE -DSUSHI_BUILD_WITH_ABLETON_LINK)
endif()

if (${WITH_VST2})
    target_compile_definitions(sushi PRIVATE -DSUSHI_BUILD_WITH_VST2 -D__cdecl=)
endif()

if (${WITH_RPC_INTERFACE})
    target_compile_definitions(sushi PRIVATE -DSUSHI_BUILD_WITH_RPC_INTERFACE)
endif()

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

install(TARGETS sushi DESTINATION bin)
foreach(ITEM ${DOC_FILES_INSTALL})
    install(FILES ${ITEM} DESTINATION ${CMAKE_INSTALL_PREFIX})
endforeach()
