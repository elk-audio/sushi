cmake_minimum_required(VERSION 2.8)
project(sushi)

####################
#  Compiler Flags  #
####################
set(PROJECT_CXX_FLAGS "-Wall -Wextra -std=c++14  -g -I/usr/xenomai/include/cobalt -I/usr/xenomai/include -D_GNU_SOURCE -D_REENTRANT -D__COBALT__ -I/usr/xenomai/include/alchemy")
#set(PROJECT_LINK_FLAGS "-lsndfile -ljsoncpp -Wl,@/usr/xenomai/lib/modechk.wrappers -lalchemy -lcopperplate -L/usr/xenomai/lib -lcobalt -lmodechk -lpthread -lrt")


# Global flags
#set(PROJECT_CXX_FLAGS "-Wall -Wextra -std=c++14 -g -I/usr/xenomai/include/cobalt -I/usr/xenomai/include -D_GNU_SOURCE -D_REENTRANT -D__COBALT__ -I/usr/xenomai/include/alchemy ")

set(PROJECT_LINK_FLAGS )

# Platform specific stuff
if (APPLE)
    set(PROJECT_LINK_FLAGS "${PROJECT_LINK_FLAGS} -framework IOKit -framework CoreFoundation")
endif()

# Build optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG   "-DDEBUG -O0 -g")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PROJECT_LINK_FLAGS}")

##################
#  Source Files  #
##################

set(COMPILATION_UNITS src/main.cpp
                      src/logging.cpp
                      src/audio_frontends/offline_frontend.cpp
#                      src/audio_frontends/jack_frontend.cpp
                      src/audio_frontends/xenomai_frontend.cpp
                      src/audio_frontends/xenomai_offline_frontend.cpp
                      src/control_frontends/base_control_frontend.cpp
                      src/control_frontends/osc_frontend.cpp
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
                      src/plugins/biquad_filter.cpp )


# Enumerate all the headers separately so that CLion can index them

set(EXTRA_CLION_SOURCES src/logging.h
                        src/options.h
                        include/plugin_interface.h
                        src/audio_frontends/base_audio_frontend.h
                        src/audio_frontends/offline_frontend.h
#                        src/audio_frontends/jack_frontend.h
                        src/audio_frontends/xenomai_frontend.h
                        src/audio_frontends/xenomai_offline_frontend.h
                        src/control_frontends/base_control_frontend.h
                        src/control_frontends/osc_frontend.h
                        src/library/sample_buffer.h
                        src/library/mind_allocator.h
                        src/library/midi_decoder.h
                        src/library/plugin_events.h
                        src/library/processor.h
                        src/library/internal_plugin.h
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
                        )

set(SOURCE_FILES "${COMPILATION_UNITS}" "${EXTRA_CLION_SOURCES}")

#########################
#  Include Directories  #
#########################

set(INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include"
                 "${PROJECT_SOURCE_DIR}/third-party/optionparser/"
                 "${PROJECT_SOURCE_DIR}/third-party/EASTL/include/"
                 "${PROJECT_SOURCE_DIR}/third-party/EASTL/test/packages/EABase/include/Common/"
                 "${PROJECT_SOURCE_DIR}/third-party/install/liblo/include"
                 "${PROJECT_SOURCE_DIR}/third-party"
                 /usr/xenomai/include
                 /usr/xenomai/include/cobalt
                 /usr/xenomai/include/xenomai)

# /usr/local doesn't get added by default in Mac OS X
if (APPLE)
    set(INCLUDE_DIRS "${INCLUDE_DIRS}" /usr/local/include)
endif()

include_directories(${INCLUDE_DIRS})

#################################
#  Linked libraries  #
#################################

set(LINK_LIBRARIES "${PROJECT_SOURCE_DIR}/third-party/install/liblo/lib/liblo.a"
                    EASTL sndfile jsoncpp alchemy copperplate cobalt pthread rt)

set(LINK_DIRS /usr/xenomai/lib)

if (APPLE)
    link_directories(${LINK_DIRS} "/usr/local/lib")
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

# Explicitly add external statically linked dependencies here,
# example:
# add_dependencies(sushi static_library)
add_dependencies(sushi liblo)

#target_link_libraries(sushi "${LINK_LIBRARIES}" jack)
target_link_libraries(sushi "${LINK_LIBRARIES}")

