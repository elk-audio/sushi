cmake_minimum_required(VERSION 3.2)
project(sushi)

##################################
#  Build  information            #
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

##################################
#  Build and link configuration  #
##################################

#  Default behaviour is to build and link with everything, using Mercury backend for Xenomai
set(WITH_XENOMAI true CACHE BOOL "Enable Xenomai support")
set(WITH_JACK true CACHE BOOL "Enable Jack support")
set(WITH_VST3 true CACHE BOOL "Enable Vst 3 support")
set(AUDIO_BUFFER_SIZE 64 CACHE STRING "Set internal audio buffer size in frames")
set(XENOMAI_AUDIO_CHANNELS 8 CACHE STRING "Set the number of audio channels in Raspa")

if (${WITH_XENOMAI})
    add_definitions(-DSUSHI_BUILD_WITH_XENOMAI)
    message("Building with Xenomai ${XENOMAI_FLAVOUR}")
endif()

if (${WITH_JACK})
    add_definitions(-DSUSHI_BUILD_WITH_JACK)
    message("Building with Jack support.")
endif()

if (${WITH_VST3})
    add_definitions(-DSUSHI_BUILD_WITH_VST3)
    message("Building with Vst 3 support.")
endif()

#############################
#  Preprocessor definitions #
#############################

# API fix for POSIX builds
add_definitions( -D__cdecl= )

####################
#  Compiler Flags  #
####################

## Global project flags.
# Change these instead than the CMAKE_*_FLAGS directly, and avoid
# putting target-specific definitions here.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Preprocessor
set(PROJECT_CXX_FLAGS "-Wall -Wextra -Wno-psabi -fno-rtti")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0"))
        set(PROJECT_CXX_FLAGS "${PROJECT_CXX_FLAGS} -faligned-new")
    endif()
endif()

# Build optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
if (${WITH_XENOMAI})
    # Additional optimizations for the Joule
    set(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE}}
            "-ftree-vectorize -ffast-math -m64 -march=corei7 -mtune=corei7 -mfpmath=sse -msse4.2")
endif()
set(CMAKE_CXX_FLAGS_DEBUG   "-DDEBUG -O0 -g")

# Linker flags
if (APPLE)
    set(LINK_DIRS ${LINK_DIRS} "/usr/local/lib")
    link_directories(${LINK_DIRS})
endif()

# Combine the results into CMake's flags.
# You shouldn't need to touch this.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PROJECT_LINK_FLAGS}")

##################
#  Source Files  #
##################

set(COMPILATION_UNITS src/main.cpp
                      src/logging.cpp
                      src/audio_frontends/offline_frontend.cpp
                      src/audio_frontends/jack_frontend.cpp
                      src/audio_frontends/xenomai_raspa_frontend.cpp
                      src/control_frontends/base_control_frontend.cpp
                      src/control_frontends/osc_frontend.cpp
                      src/dsp_library/biquad_filter.cpp
                      src/engine/audio_engine.cpp
                      src/engine/event_dispatcher.cpp
                      src/engine/track.cpp
                      src/engine/midi_dispatcher.cpp
                      src/engine/json_configurator.cpp
                      src/engine/receiver.cpp
                      src/engine/event_timer.cpp
                      src/engine/transport.cpp
                      src/library/event.cpp
                      src/library/mind_allocator.cpp
                      src/library/midi_decoder.cpp
                      src/library/midi_encoder.cpp
                      src/library/internal_plugin.cpp
                      src/library/vst2x_wrapper.cpp
                      src/library/vst2x_host_callback.cpp
                      src/library/vst2x_plugin_loader.cpp
                      src/library/vst3x_wrapper.cpp
                      src/plugins/arpeggiator_plugin.cpp
                      src/plugins/gain_plugin.cpp
                      src/plugins/passthrough_plugin.cpp
                      src/plugins/equalizer_plugin.cpp
                      src/plugins/peak_meter_plugin.cpp
                      src/plugins/sample_player_plugin.cpp
                      src/plugins/sample_player_voice.cpp
                      src/audio_frontends/offline_frontend.cpp
        )

# Enumerate all the headers separately so that CLion can index them

set(EXTRA_CLION_SOURCES src/logging.h
                        src/options.h
                        include/plugin_interface.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/audio_frontend_internals.h
                        src/audio_frontends/offline_frontend.h
                        src/audio_frontends/jack_frontend.h
                        src/audio_frontends/xenomai_raspa_frontend.h
                        src/control_frontends/base_control_frontend.h
                        src/control_frontends/osc_frontend.h
                        src/control_frontends/base_midi_frontend.h
                        src/dsp_library/envelopes.h
                        src/dsp_library/sample_wrapper.h
                        src/dsp_library/biquad_filter.h
                        src/library/event.h
                        src/library/event_interface.h
                        src/library/sample_buffer.h
                        src/library/mind_allocator.h
                        src/library/midi_decoder.h
                        src/library/midi_encoder.h
                        src/library/rt_event.h
                        src/library/processor.h
                        src/library/internal_plugin.h
                        src/library/rt_event_fifo.h
                        src/library/rt_event_pipe.h
                        src/library/synchronised_fifo.h
                        src/library/time.h
                        src/library/vst2x_wrapper.h
                        src/library/vst2x_midi_event_fifo.h
                        src/library/vst2x_host_callback.h
                        src/library/vst2x_plugin_loader.h
                        src/library/vst3x_wrapper.h
                        src/engine/base_engine.h
                        src/engine/audio_engine.h
                        src/engine/track.h
                        src/engine/receiver.h
                        src/engine/midi_dispatcher.h
                        src/engine/event_dispatcher.h
                        src/engine/event_timer.h
                        src/engine/json_configurator.h
                        src/engine/midi_receiver.h
                        src/engine/host_control.h
                        src/engine/transport.h
                        src/plugins/arpeggiator_plugin.h
                        src/plugins/gain_plugin.h
                        src/plugins/passthrough_plugin.h
                        src/plugins/equalizer_plugin.h
                        src/plugins/peak_meter_plugin.h
                        src/plugins/sample_player_plugin.h
                        src/plugins/sample_player_voice.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/offline_frontend.h
        )

set(ADDITIONAL_VST3_SOURCES src/library/vst3x_host_app.cpp
                            src/library/vst3x_utils.cpp
                            src/library/vst3x_host_app.h
                            src/library/vst3x_utils.h)

set(SOURCE_FILES "${COMPILATION_UNITS}" "${PLATFORM_SPECIFIC_SOURCES}" "${EXTRA_CLION_SOURCES}")

if (${WITH_XENOMAI} OR ${WITH_JACK})
    set(SOURCE_FILES "${SOURCE_FILES}" src/control_frontends/alsa_midi_frontend.h
                                       src/control_frontends/alsa_midi_frontend.cpp)
endif()

if (${WITH_VST3})
    set(SOURCE_FILES "${SOURCE_FILES}" "${ADDITIONAL_VST3_SOURCES}")
endif()

#########################
#  Include Directories  #
#########################

set(VST_SDK_PATH "${PROJECT_SOURCE_DIR}/third-party/vstsdk2.4")

set(INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include"
                 "${CMAKE_BINARY_DIR}" # for generated version.h
                 "${PROJECT_SOURCE_DIR}/raspalib/include"
                 "${PROJECT_SOURCE_DIR}/twine/include"
                 "${PROJECT_SOURCE_DIR}"
                 "${PROJECT_SOURCE_DIR}/third-party/optionparser/"
                 "${PROJECT_SOURCE_DIR}/third-party/rapidjson/include"
                 "${PROJECT_SOURCE_DIR}/third-party/EASTL/include/"
                 "${PROJECT_SOURCE_DIR}/third-party/EASTL/test/packages/EABase/include/Common/"
                 "${VST_SDK_PATH}/pluginterfaces/vst2.x"
                 "${PROJECT_SOURCE_DIR}/third-party/install/liblo/include"
                 "${PROJECT_SOURCE_DIR}/third-party/spdlog/include"
                 "${PROJECT_SOURCE_DIR}/third-party/vstsdk3"
                 "${PROJECT_SOURCE_DIR}/third-party")

# /usr/local doesn't get added by default in Mac OS X
if (APPLE)
    set(INCLUDE_DIRS "${INCLUDE_DIRS}" /usr/local/include)
endif()


#################################
#  Linked libraries             #
#################################

# Common libraries between all builds, and safe to be included in test targets
set(COMMON_LIBRARIES
    EASTL
    sndfile
    lo
    pthread
    dl
    twine
)

if (${WITH_VST3})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} vst3_host sdk)
endif()

if (${WITH_JACK})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} jack asound)
endif()

if (${WITH_XENOMAI})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} raspa asound)
endif()

#############
# Vst3 Host #
#############

# Build the given Vst3 host implementation into a separate library to
# Avoid littering our own code with ugly defines

set(VST3_SDK_PATH "${PROJECT_SOURCE_DIR}/third-party/vstsdk3")
set(VST3_HOST_SOURCES
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/eventlist.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/parameterchanges.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/hostclasses.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module_linux.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/stringconvert.cpp")

add_library(vst3_host STATIC ${VST3_HOST_SOURCES})
set_target_properties(vst3_host PROPERTIES EXCLUDE_FROM_ALL true)
set_target_properties(vst3_host PROPERTIES COMPILE_FLAGS "-DDEVELOPMENT -Wno-unused-parameter -Wno-extra")
target_include_directories(vst3_host PRIVATE "${PROJECT_SOURCE_DIR}/third-party/vstsdk3")
add_dependencies(vst3_host sdk)

#############################################
#  Subdirectory projects                    #
#############################################
if (${WITH_XENOMAI})
    set(WITH_RASPA_APPS false)
    set(WITH_RASPA_TESTS false)
    set(WITH_TWINE_XENOMAI true)
    add_definitions(-DRASPA_N_FRAMES_PER_BUFFER=${AUDIO_BUFFER_SIZE})
    add_definitions(-DRASPA_N_CHANNELS=${XENOMAI_AUDIO_CHANNELS})
    add_subdirectory(raspalib)

    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} asound)
endif()

set(WITH_TWINE_TESTS false)
add_subdirectory(twine)

add_subdirectory(third-party EXCLUDE_FROM_ALL)
add_subdirectory(test)

####################
#  Target Objects  #
####################
add_executable(sushi ${SOURCE_FILES})
target_compile_definitions(sushi PRIVATE -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=${AUDIO_BUFFER_SIZE})
target_include_directories(sushi PRIVATE ${INCLUDE_DIRS})
target_link_libraries(sushi PRIVATE ${EXTRA_BUILD_LIBRARIES} ${COMMON_LIBRARIES})

if (${WITH_VST3})
    add_dependencies(sushi vst3_host sdk base)
endif()

####################
#  Install         #
####################
set(DOC_FILES_INSTALL
    "${PROJECT_SOURCE_DIR}/LICENSE"
    "${PROJECT_SOURCE_DIR}/README.md"
    "${PROJECT_SOURCE_DIR}/HISTORY.md"
)
set(EXAMPLE_CONFIGS_INSTALL
    "${PROJECT_SOURCE_DIR}/misc/config_files/config_empty.json"
    "${PROJECT_SOURCE_DIR}/misc/config_files/config_fx.json"
    "${PROJECT_SOURCE_DIR}/misc/config_files/config_synth.json"
    "${PROJECT_SOURCE_DIR}/misc/config_files/config_multi_track.json"
    "${PROJECT_SOURCE_DIR}/misc/config_files/config_multichannel_plugin.json"
    "${PROJECT_SOURCE_DIR}/test/data/config.json"
)

install(TARGETS sushi DESTINATION bin)
foreach(ITEM ${DOC_FILES_INSTALL})
    install(FILES ${ITEM} DESTINATION ${CMAKE_INSTALL_PREFIX})
endforeach()
foreach(ITEM ${EXAMPLE_CONFIGS_INSTALL})
    install(FILES ${ITEM} DESTINATION example_configs)
endforeach()

