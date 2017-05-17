cmake_minimum_required(VERSION 3.2)
project(sushi)

##################################
#  Build and link configuration  #
##################################

#  Default behaviour is to build and link with everything
if (DEFINED BUILD_WITH_XENOMAI)
    set(BUILD_WITH_XENOMAI ${BUILD_WITH_XENOMAI} CACHE BOOL "" )
    if (DEFINED XENOMAI_FLAVOUR)
        set(XENOMAI_FLAVOUR  ${XENOMAI_FLAVOUR} CACHE BOOL "")
    else()
        set(XENOMAI_FLAVOUR mercury CACHE STRING "")
    endif()
else()
    set(BUILD_WITH_XENOMAI true CACHE BOOL "")
    set(XENOMAI_FLAVOUR mercury CACHE STRING "")
endif()

if (DEFINED BUILD_WITH_JACK)
    set(BUILD_WITH_JACK ${BUILD_WITH_JACK} CACHE BOOL "")
else()
    set(BUILD_WITH_JACK true CACHE BOOL "")
endif()

if(${BUILD_WITH_JACK})
    add_definitions(-DSUSHI_BUILD_WITH_JACK)
    message("Building with Jack support.")
endif()
if(${BUILD_WITH_XENOMAI})
    add_definitions(-DSUSHI_BUILD_WITH_XENOMAI)
    message("Building with Xenomai ${XENOMAI_FLAVOUR}")
endif()

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

if (${BUILD_WITH_XENOMAI} AND ${XENOMAI_FLAVOUR} EQUAL "cobalt")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -D_REENTRANT -D__COBALT__ ")
elseif(${BUILD_WITH_XENOMAI} AND ${XENOMAI_FLAVOUR} EQUAL "mercury")
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
                      src/library/mind_allocator.cpp
                      src/library/midi_decoder.cpp
                      src/library/internal_plugin.cpp
                      src/plugins/gain_plugin.cpp
                      src/plugins/passthrough_plugin.cpp
                      src/plugins/equalizer_plugin.cpp
                      src/plugins/sample_player_plugin.cpp
                      src/plugins/sample_player_voice.cpp
                      src/plugins/biquad_filter.cpp
                      src/audio_frontends/offline_frontend.cpp
                      #src/library/vst2x_plugin.cpp
                      src/library/vst2x_host_callback.cpp
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
                        src/library/vst2x_plugin_loader.h
                        src/library/event_fifo.h
                        src/library/event_pipe.h
                        src/engine/engine.h
                        src/engine/plugin_chain.h
                        src/engine/midi_dispatcher.h
                        src/plugins/gain_plugin.h
                        src/plugins/passthrough_plugin.h
                        src/plugins/equalizer_plugin.h
                        src/plugins/sample_player_plugin.h
                        src/plugins/sample_player_voice.h
                        src/plugins/biquad_filter.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/offline_frontend.h
                        #src/library/vst2x_plugin.h
                        src/library/vst2x_host_callback.h
        )


# Platform-specific sources
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(PLATFORM_SPECIFIC_SOURCES
        src/library/vst2x_plugin_loader_linux.cpp
    )

elseif(APPLE)
    set(PLATFORM_SPECIFIC_SOURCES
        src/library/vst2x_plugin_loader_mac.mm
    )
endif()

if(APPLE)
    set_source_files_properties(${PLATFORM_SPECIFIC_SOURCES}
        PROPERTIES COMPILE_FLAGS
        "-x objective-c++"
    )
endif()

set(SOURCE_FILES "${COMPILATION_UNITS}" "${PLATFORM_SPECIFIC_SOURCES}" "${EXTRA_CLION_SOURCES}")

#########################
#  Include Directories  #
#########################

set(VST_SDK_PATH "${PROJECT_SOURCE_DIR}/third-party/vstsdk2.4")

set(INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include"
                 "${PROJECT_SOURCE_DIR}/third-party/optionparser/"
                 "${PROJECT_SOURCE_DIR}/third-party/EASTL/include/"
                 "${PROJECT_SOURCE_DIR}/third-party/EASTL/test/packages/EABase/include/Common/"
                 "${VST_SDK_PATH}/pluginterfaces/vst2.x"
                 "${PROJECT_SOURCE_DIR}/third-party/install/liblo/include"
                 "${PROJECT_SOURCE_DIR}/third-party")

if(${BUILD_WITH_XENOMAI})
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
                    EASTL sndfile jsoncpp pthread)

if (BUILD_WITH_XENOMAI)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} alchemy copperplate ${XENOMAI_FLAVOUR} pthread rt)
    set(LINK_DIRS /usr/xenomai/lib)
else()
    set(LINK_DIRS "")
endif()

if (APPLE)
    # set(PROJECT_LINK_FLAGS "${PROJECT_LINK_FLAGS} -framework IOKit -framework CoreFoundation")
    set(PROJECT_LINK_FLAGS "${PROJECT_LINK_FLAGS}"
                           "-framework AppKit"
                           "-framework Carbon"
                           "-framework CoreFoundation"
                           "-framework Foundation"
        )
    set(LINK_DIRS ${LINK_DIRS} "/usr/local/lib")
endif()

link_directories(${LINK_DIRS})


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
add_dependencies(sushi liblo)

if (${BUILD_WITH_JACK})
    set(LINK_LIBRARIES ${LINK_LIBRARIES} jack)
endif()

target_link_libraries(sushi ${LINK_LIBRARIES})

