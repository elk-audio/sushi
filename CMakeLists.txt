cmake_minimum_required(VERSION 3.2)
project(sushi)

##################################
#  Build and link configuration  #
##################################

#  Default behaviour is to build and link with everything, using Mercury backend for Xenomai
set(WITH_XENOMAI true CACHE BOOL "Enable Xenomai support")
set(XENOMAI_FLAVOUR mercury CACHE STRING "Xenomai backend flavour. Options are: cobalt mercury")
set(WITH_JACK true CACHE BOOL "Enable Jack support")

if (${WITH_XENOMAI})
    add_definitions(-DSUSHI_BUILD_WITH_XENOMAI)
    message("Building with Xenomai ${XENOMAI_FLAVOUR}")
endif()

if (${WITH_JACK})
    add_definitions(-DSUSHI_BUILD_WITH_JACK)
    message("Building with Jack support.")
endif()

#############################
#  Preprocessor definitions #
#############################

# API fix for POSIX builds
add_definitions( -D__cdecl= )

####################
#  Compiler Flags  #
####################

# Global flags
set(PROJECT_CXX_FLAGS "-Wall -Wextra -Wno-psabi -std=c++17 -fno-rtti")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0"))
        set(PROJECT_CXX_FLAGS "${PROJECT_CXX_FLAGS} -faligned-new")
    endif()
endif()

set(PROJECT_LINK_FLAGS )


# Build optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG   "-DDEBUG -O0 -g")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PROJECT_LINK_FLAGS}")

if (${WITH_XENOMAI} AND ${XENOMAI_FLAVOUR} EQUAL "cobalt")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -D_REENTRANT -D__COBALT__ ")
elseif(${WITH_XENOMAI} AND ${XENOMAI_FLAVOUR} EQUAL "mercury")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -D_REENTRANT -D__MERCURY__ ")
endif()

##################
#  Source Files  #
##################

set(COMPILATION_UNITS src/main.cpp
                      src/logging.cpp
                      src/audio_frontends/offline_frontend.cpp
                      src/audio_frontends/jack_frontend.cpp
                      src/audio_frontends/xenomai_frontend.cpp
                      src/control_frontends/base_control_frontend.cpp
                      src/control_frontends/osc_frontend.cpp
                      src/dsp_library/biquad_filter.cpp
                      src/engine/engine.cpp
                      src/engine/plugin_chain.cpp
                      src/engine/midi_dispatcher.cpp
                      src/engine/json_configurator.cpp
                      src/library/mind_allocator.cpp
                      src/library/midi_decoder.cpp
                      src/library/internal_plugin.cpp
                      src/library/vst2x_wrapper.cpp
                      src/library/vst2x_host_callback.cpp
                      src/library/vst2x_plugin_loader.cpp
                      src/library/vst3x_utils.cpp
                      src/library/vst3x_wrapper.cpp
                      src/library/vst3x_host_app.cpp
                      src/plugins/gain_plugin.cpp
                      src/plugins/passthrough_plugin.cpp
                      src/plugins/equalizer_plugin.cpp
                      src/plugins/sample_player_plugin.cpp
                      src/plugins/sample_player_voice.cpp
                      src/audio_frontends/offline_frontend.cpp
        )

# Enumerate all the headers separately so that CLion can index them

set(EXTRA_CLION_SOURCES src/logging.h
                        src/options.h
                        include/plugin_interface.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/offline_frontend.h
                        src/audio_frontends/jack_frontend.h
                        src/audio_frontends/xenomai_frontend.h
                        src/control_frontends/base_control_frontend.h
                        src/control_frontends/osc_frontend.h
                        src/dsp_library/envelopes.h
                        src/dsp_library/sample_wrapper.h
                        src/dsp_library/biquad_filter.h
                        src/library/sample_buffer.h
                        src/library/mind_allocator.h
                        src/library/midi_decoder.h
                        src/library/plugin_events.h
                        src/library/processor.h
                        src/library/internal_plugin.h
                        src/library/event_fifo.h
                        src/library/event_pipe.h
                        src/library/vst2x_wrapper.h
                        src/library/vst2x_midi_event_fifo.h
                        src/library/vst2x_host_callback.h
                        src/library/vst2x_plugin_loader.h
                        src/library/vst3x_wrapper.h
                        src/library/vst3x_host_app.h
                        src/library/vst3x_utils.h
                        src/engine/engine.h
                        src/engine/plugin_chain.h
                        src/engine/midi_dispatcher.h
                        src/engine/json_configurator.h
                        src/plugins/gain_plugin.h
                        src/plugins/passthrough_plugin.h
                        src/plugins/equalizer_plugin.h
                        src/plugins/sample_player_plugin.h
                        src/plugins/sample_player_voice.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/offline_frontend.h
        )

set(SOURCE_FILES "${COMPILATION_UNITS}" "${PLATFORM_SPECIFIC_SOURCES}" "${EXTRA_CLION_SOURCES}")

#########################
#  Include Directories  #
#########################

set(VST_SDK_PATH "${PROJECT_SOURCE_DIR}/third-party/vstsdk2.4")

set(INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include"
                 "${PROJECT_SOURCE_DIR}/third-party/optionparser/"
                 "${PROJECT_SOURCE_DIR}/third-party/rapidjson/include"
                 "${PROJECT_SOURCE_DIR}/third-party/EASTL/include/"
                 "${PROJECT_SOURCE_DIR}/third-party/EASTL/test/packages/EABase/include/Common/"
                 "${VST_SDK_PATH}/pluginterfaces/vst2.x"
                 "${PROJECT_SOURCE_DIR}/third-party/install/liblo/include"
                 "${PROJECT_SOURCE_DIR}/third-party/vstsdk3"
                 "${PROJECT_SOURCE_DIR}/third-party")

if(${WITH_XENOMAI})
    set(INCLUDE_DIRS ${INCLUDE_DIRS} /usr/xenomai/include
                                     /usr/xenomai/include/${XENOMAI_FLAVOUR}
                                     /usr/xenomai/include/xenomai
                                     /usr/xenomai/include/alchemy)
endif()

# /usr/local doesn't get added by default in Mac OS X
if (APPLE)
    set(INCLUDE_DIRS "${INCLUDE_DIRS}" /usr/local/include)
endif()

include_directories(${INCLUDE_DIRS})

#################################
#  Linked libraries  #
#################################

set(LINK_LIBRARIES "${PROJECT_SOURCE_DIR}/third-party/install/liblo/lib/liblo.a"
                    EASTL sndfile jsoncpp pthread dl vst3_host sdk )

if (${WITH_XENOMAI})
    set(LINK_LIBRARIES ${LINK_LIBRARIES} alchemy copperplate ${XENOMAI_FLAVOUR} pthread rt)
    set(LINK_DIRS /usr/xenomai/lib)
else()
    set(LINK_DIRS "")
endif()

if (APPLE)
    set(PROJECT_LINK_FLAGS "${PROJECT_LINK_FLAGS}"
                           "-framework IOKit"
                           "-framework CoreFoundation"
        )
    set(LINK_DIRS ${LINK_DIRS} "/usr/local/lib")
endif()

link_directories(${LINK_DIRS})

#############
# Vst3 Host #
#############

# Build the given Vst3 host implementation into a separate library to
# Avoid littering out own code with ugly defines

set(VST3_SDK_PATH "${PROJECT_SOURCE_DIR}/third-party/vstsdk3")
set(VST3_HOST_SOURCES
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/eventlist.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/parameterchanges.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/hostclasses.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module_linux.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module.cpp"
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/stringconvert.cpp")

add_library(vst3_host STATIC ${VST3_HOST_SOURCES})
set_target_properties(vst3_host PROPERTIES COMPILE_FLAGS -DDEVELOPMENT)
add_dependencies(vst3_host sdk)

#############################################
#  Subdirectory projects                    #
#############################################

add_subdirectory(third-party)
add_subdirectory(test)

####################
#  Target Objects  #
####################

add_executable(sushi ${SOURCE_FILES})

# example:
# add_dependencies(sushi static_library)
add_dependencies(sushi liblo vst3_host sdk base)

if (${WITH_JACK})
    set(LINK_LIBRARIES ${LINK_LIBRARIES} jack)
endif()

target_link_libraries(sushi ${LINK_LIBRARIES})

