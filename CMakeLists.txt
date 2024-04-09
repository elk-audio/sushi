cmake_minimum_required(VERSION 3.8..3.23)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

#####################
#  Project Version  #
#####################

# Don't change version anywhere else. Everything is generated from this.
set(SUSHI_VERSION_MAJOR 1)
set(SUSHI_VERSION_MINOR 1)
set(SUSHI_VERSION_REVISION 0)

project(sushi_library
    DESCRIPTION "Headless plugin host for Elk Audio OS"
    HOMEPAGE_URL "https://github.com/elk-audio/sushi"
    LANGUAGES CXX
    VERSION ${SUSHI_VERSION_MAJOR}.${SUSHI_VERSION_MINOR}.${SUSHI_VERSION_REVISION}
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

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

# The defaults enable all options and select APIs available for either Xenomai or macOS
set(SUSHI_WITH_JACK_DEFAULT OFF)
set(SUSHI_WITH_VST2_DEFAULT OFF)
set(SUSHI_WITH_VST3_DEFAULT ON)
set(SUSHI_LINK_VST3_DEFAULT ON)
set(SUSHI_WITH_LINK_DEFAULT ON)
set(SUSHI_WITH_RPC_INTERFACE_DEFAULT ON)
set(SUSHI_TWINE_STATIC_DEFAULT OFF)
set(SUSHI_AUDIO_BUFFER_SIZE_DEFAULT 64)
set(SUSHI_WITH_SENTRY_DEFAULT OFF)
set(SUSHI_SENTRY_DSN_DEFAULT "--Sentry default DSN is undefined--")
set(SUSHI_DISABLE_MULTICORE_UNIT_TESTS_DEFAULT OFF)
set(SUSHI_BUILD_STANDALONE_APP_DEFAULT ON)
set(SUSHI_BUILD_WITH_SANITIZERS_DEFAULT OFF)

set(SUSHI_RASPA_FLAVOR_DEFAULT "evl")
set(TWINE_WITH_XENOMAI_DEFAULT OFF)
set(TWINE_WITH_EVL_DEFAULT OFF)

if (APPLE)
    # macOS defaults
    set(SUSHI_WITH_RASPA_DEFAULT OFF)
    set(SUSHI_WITH_PORTAUDIO_DEFAULT OFF)
    set(SUSHI_WITH_APPLE_COREAUDIO_DEFAULT ON)
    set(SUSHI_WITH_ALSA_MIDI_DEFAULT OFF)
    set(SUSHI_WITH_RT_MIDI_DEFAULT ON)
    set(SUSHI_WITH_VST2_DEFAULT OFF)
    set(SUSHI_WITH_LV2_DEFAULT OFF)
    set(SUSHI_WITH_LV2_MDA_TESTS_DEFAULT OFF)
    set(SUSHI_WITH_UNIT_TESTS_DEFAULT ON)
    set(SUSHI_BUILD_TWINE_DEFAULT ON)
    set(SUSHI_DISABLE_MULTICORE_UNIT_TESTS_DEFAULT ON)
elseif (CMAKE_CROSSCOMPILING)
    # Yocto cross-compilation defaults
    set(SUSHI_WITH_RASPA_DEFAULT ON)
    set(SUSHI_WITH_PORTAUDIO_DEFAULT OFF)
    set(SUSHI_WITH_APPLE_COREAUDIO_DEFAULT OFF)
    set(SUSHI_WITH_ALSA_MIDI_DEFAULT ON)
    set(SUSHI_WITH_RT_MIDI_DEFAULT OFF)
    set(SUSHI_WITH_LV2_DEFAULT ON)
    set(SUSHI_WITH_LV2_MDA_TESTS_DEFAULT OFF)
    set(SUSHI_WITH_UNIT_TESTS_DEFAULT OFF)
    set(SUSHI_BUILD_TWINE_DEFAULT OFF)
else()
    # Native Linux defaults
    set(SUSHI_WITH_JACK_DEFAULT ON)
    set(SUSHI_WITH_RASPA_DEFAULT OFF)
    set(SUSHI_WITH_PORTAUDIO_DEFAULT OFF)
    set(SUSHI_WITH_APPLE_COREAUDIO_DEFAULT OFF)
    set(SUSHI_WITH_ALSA_MIDI_DEFAULT ON)
    set(SUSHI_WITH_RT_MIDI_DEFAULT OFF)
    set(SUSHI_WITH_LV2_DEFAULT ON)
    set(SUSHI_WITH_LV2_MDA_TESTS_DEFAULT ON)
    set(SUSHI_WITH_UNIT_TESTS_DEFAULT ON)
    set(SUSHI_BUILD_TWINE_DEFAULT ON)
endif()

option(SUSHI_WITH_RASPA "Enable Raspa (xenomai) support" ${SUSHI_WITH_RASPA_DEFAULT})
option(SUSHI_WITH_JACK "Enable Jack support" ${SUSHI_WITH_JACK_DEFAULT})
option(SUSHI_WITH_PORTAUDIO "Enable PortAudio support" ${SUSHI_WITH_PORTAUDIO_DEFAULT})
option(SUSHI_WITH_APPLE_COREAUDIO "Enable Apple CoreAudio support" ${SUSHI_WITH_APPLE_COREAUDIO_DEFAULT})
option(SUSHI_WITH_ALSA_MIDI "Enable alsa midi support" ${SUSHI_WITH_ALSA_MIDI_DEFAULT})
option(SUSHI_WITH_RT_MIDI "Enable RtMidi support" ${SUSHI_WITH_RT_MIDI_DEFAULT})
option(SUSHI_WITH_VST2 "Enable Vst 2 support" ${SUSHI_WITH_VST2_DEFAULT})
option(SUSHI_WITH_VST3 "Enable Vst 3 support" ${SUSHI_WITH_VST3_DEFAULT})
option(SUSHI_LINK_VST3 "Link Vst 3 library to Sushi" ${SUSHI_LINK_VST3_DEFAULT})
option(SUSHI_WITH_LV2 "Enable LV2 support" ${SUSHI_WITH_LV2_DEFAULT})
option(SUSHI_WITH_LV2_MDA_TESTS "Include unit tests depending on LV2 drobilla MDA plugin port." ${SUSHI_WITH_LV2_MDA_TESTS_DEFAULT})
option(SUSHI_WITH_UNIT_TESTS "Build and run unit tests after compilation" ${SUSHI_WITH_UNIT_TESTS_DEFAULT})
option(SUSHI_WITH_LINK "Enable Ableton Link support" ${SUSHI_WITH_LINK_DEFAULT})
option(SUSHI_WITH_RPC_INTERFACE "Enable RPC control support" ${SUSHI_WITH_RPC_INTERFACE_DEFAULT})
option(SUSHI_BUILD_TWINE "Build included Twine library" ${SUSHI_BUILD_TWINE_DEFAULT})
option(SUSHI_TWINE_STATIC "Link against static version of TWINE library" ${SUSHI_TWINE_STATIC_DEFAULT})
option(SUSHI_WITH_SENTRY "Enable sentry.io support" ${SUSHI_WITH_SENTRY_DEFAULT})
option(SUSHI_SENTRY_DSN "Required Sentry DSN Path" ${SUSHI_SENTRY_DSN_DEFAULT})
option(SUSHI_DISABLE_MULTICORE_UNIT_TESTS "Disable unit-tests dependent on multi-core processing." ${SUSHI_DISABLE_MULTICORE_UNIT_TESTS_DEFAULT})
option(SUSHI_BUILD_STANDALONE_APP "Build standalone Sushi executable" ${SUSHI_BUILD_STANDALONE_APP_DEFAULT})
option(SUSHI_BUILD_WITH_SANITIZERS "Build Sushi with google address sanitizer on" ${SUSHI_BUILD_WITH_SANITIZERS_DEFAULT})

set(SUSHI_AUDIO_BUFFER_SIZE ${SUSHI_AUDIO_BUFFER_SIZE_DEFAULT} CACHE STRING "Set internal audio buffer size in frames")
set(SUSHI_RASPA_FLAVOR ${SUSHI_RASPA_FLAVOR_DEFAULT} CACHE STRING "Set raspa flavor. Options are xenomai or evl")

if (SUSHI_WITH_ALSA_MIDI AND SUSHI_WITH_RT_MIDI)
    message(FATAL_ERROR "Both alsa midi and RtMidi set to on. Use only one midi frontend at a time")
endif()

if (SUSHI_WITH_RASPA)
    # Determine raspa evl/xenomai variables
    if (${SUSHI_RASPA_FLAVOR} MATCHES "evl")
        message("Building with Raspa/Twine EVL support")
        set(TWINE_WITH_EVL ON CACHE BOOL "" FORCE)
    elseif (${SUSHI_RASPA_FLAVOR} MATCHES "xenomai")
        message("Building with Raspa/Twine Xenomai support.")
        message("Warning Xenomai 3 support is deprecated and will be removed in future versions")
        set(TWINE_WITH_XENOMAI ON CACHE BOOL "" FORCE)
    else ()
        message(FATAL_ERROR "Raspa flavor should be set to xenomai or evl")
    endif()
endif()

###############################
# Sub-directory configuration #
###############################

if (${SUSHI_BUILD_TWINE})
    set(TWINE_WITH_TESTS OFF CACHE BOOL "")
    add_subdirectory(twine)

    if (${SUSHI_TWINE_STATIC})
        set(TWINE_LIB twine::twine_static)
    else()
        set(TWINE_LIB twine::twine)
    endif()
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${TWINE_LIB})
else()
    find_package(twine 0.4)
    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${TWINE_PATH})
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${TWINE_LIB})
endif()

set(ELKLOG_WITH_UNIT_TESTS OFF CACHE BOOL "")
set(ELKLOG_USE_INCLUDED_TWINE OFF CACHE BOOL "")
set(ELKLOG_WITH_EXAMPLES OFF CACHE BOOL "")
set(ELKLOG_MULTITHREADED_RT_LOGGING OFF CACHE BOOL "")
set(ELKLOG_FILE_SIZE 10000000 CACHE STRING "")
set(ELKLOG_RT_MESSAGE_SIZE 2048 CACHE STRING "")
set(ELKLOG_RT_QUEUE_SIZE 1024 CACHE STRING "")

add_subdirectory(elklog)

add_subdirectory(third-party EXCLUDE_FROM_ALL)

###############
# Raspa setup #
###############

if (${SUSHI_WITH_RASPA})
    find_library(RASPA_LIB
            NAMES raspa
            PATHS /usr/lib /usr/local/lib
            )
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${RASPA_LIB} asound)
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_RASPA)

    if (NOT "$ENV{CMAKE_SYSROOT}" STREQUAL "")
        set(CMAKE_SYSROOT "$ENV{CMAKE_SYSROOT}")
        message("ENV_CMAKE_SYSROOT = " $ENV{CMAKE_SYSROOT})
    endif()

    # Determine whether to link with evl or cobalt lib
    if (${SUSHI_RASPA_FLAVOR} MATCHES "xenomai")

        # External libraries:
        # explicitly link with cobalt lib
        set(XENOMAI_BASE_DIR "/usr/xenomai" CACHE STRING "xenomai base dir path")

        if (NOT "$ENV{CMAKE_SYSROOT}" STREQUAL "")
            set(CMAKE_SYSROOT "$ENV{CMAKE_SYSROOT}")
            message("ENV_CMAKE_SYSROOT = " $ENV{CMAKE_SYSROOT})
        endif()

        if (NOT "${CMAKE_SYSROOT}" STREQUAL "")
            set(XENOMAI_BASE_DIR "${CMAKE_SYSROOT}/usr/xenomai")
            message("XENOMAI_BASE_DIR is " ${XENOMAI_BASE_DIR})
        endif()

        find_library(COBALT_LIB cobalt HINTS ${XENOMAI_BASE_DIR}/lib)
        set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${COBALT_LIB})

    elseif (${SUSHI_RASPA_FLAVOR} MATCHES "evl")
        message("Building with EVL support")
        find_library(EVL_LIB
            NAMES evl
            PATHS /usr/lib /usr/local/lib
            )
        set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} ${EVL_LIB})
    endif()
endif()

##############
# Jack setup #
##############

if (${SUSHI_WITH_JACK})
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

if (${SUSHI_WITH_PORTAUDIO})
    message("Building with PortAudio support.")

    find_package(portaudio CONFIG REQUIRED)

    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} portaudio_static)

    # Compile definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_PORTAUDIO)
endif()

if (APPLE)
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_APPLE_THREADING)

    ###################
    # CoreAudio setup #
    ###################

    if (${SUSHI_WITH_APPLE_COREAUDIO})
        message("Building with Apple CoreAudio support.")

        # Compile definitions
        set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_APPLE_COREAUDIO)

        set(ADDITIONAL_APPLE_COREAUDIO_SOURCES  src/audio_frontends/apple_coreaudio/apple_coreaudio_utils.cpp
                                                src/audio_frontends/apple_coreaudio/apple_coreaudio_object.cpp
                                                src/audio_frontends/apple_coreaudio/apple_coreaudio_device.mm)

        set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} "-framework CoreAudio -framework Foundation")
    endif()
endif()

###################
# Alsa midi setup #
###################

if (${SUSHI_WITH_ALSA_MIDI})
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

################
# RtMidi setup #
################

if (${SUSHI_WITH_RT_MIDI})
    message("Building with RtMidi support.")

    # Additional sources
    set(MIDI_SOURCES ${MIDI_SOURCES} src/control_frontends/rt_midi_frontend.cpp src/control_frontends/rt_midi_frontend.h)

    #RtMidi library
    find_package(RtMidi CONFIG REQUIRED)

    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} RtMidi::rtmidi)

    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_RT_MIDI)
endif()

###############
# VST 2 setup #
###############

if (${SUSHI_WITH_VST2})
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
        set(SUSHI_VST2_SDK_PATH ${VST_2_SDK_BASE_DIR})
    else()
        find_file(SUSHI_VST2_SDK_PATH NAMES VST2_SDK vstsdk2.4 HINTS $ENV{HOME}/workspace
                                                                     $ENV{HOME}/workspace/VST_SDK
                                                                     $ENV{HOME}/workspace/vstsdk3610_11_06_2018_build_37)
    endif()
    if (${SUSHI_VST2_SDK_PATH} STREQUAL "VST2_SDK_PATH-NOTFOUND")
        message("Warning! Could not find Steinberg SDK for VST 2.4")
    endif()

    set(INCLUDE_DIRS ${INCLUDE_DIRS} ${SUSHI_VST2_SDK_PATH}/pluginterfaces/vst2.x)

    # Compile definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_VST2 -D__cdecl=)
endif()

###############
# VST 3 setup #
###############

# TODO: use this ${SUSHI_LINK_VST3}, and when it's off, link to VST3 SDK included by JUCE.
if (${SUSHI_WITH_VST3})
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

    set(VST3_COMPILE_FLAGS "-Wno-unused-parameter \
                            -Wno-extra \
                            -Wno-deprecated \
                            -Wno-cpp \
                            -Wno-pointer-bool-conversion \
                            -Wno-suggest-override \
                            -Wno-deprecated-declarations \
                            -Wno-shorten-64-to-32 \
                            -Wformat=0")

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(VST3_HOST_SOURCES ${VST3_HOST_SOURCES} "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module_linux.cpp")
        set(AUDIOHOST_PLATFORM_LIBS)
    endif()

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        set(VST3_HOST_SOURCES
            ${VST3_HOST_SOURCES}
            "${VST3_SDK_PATH}/public.sdk/source/vst/hosting/module_mac.mm")
        set(AUDIOHOST_PLATFORM_LIBS "-framework Cocoa")
        set(VST3_COMPILE_FLAGS "${VST3_COMPILE_FLAGS} -fobjc-arc -Wno-unqualified-std-cast-call")
    endif()

    add_library(vst3_host STATIC ${VST3_HOST_SOURCES})
    target_link_libraries(vst3_host PRIVATE ${AUDIOHOST_PLATFORM_LIBS})
    set_target_properties(vst3_host PROPERTIES EXCLUDE_FROM_ALL true)
    target_compile_features(vst3_host PUBLIC cxx_std_20)
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


    # In yocto env / cross compiling scenarios, this is not needed as mda-vst3 plugins are
    # built already. Also, this causes an "exec-format" error since the plugins need a specific
    # method of compilation during cross compilation process.
    if (NOT CMAKE_CROSSCOMPILING)
        # Triggering mda-vst3 build - used by sample sushi json configurations under misc/config_files.
        set_target_properties(mda-vst3 PROPERTIES EXCLUDE_FROM_ALL False)
    endif()
endif()

#############
# LV2 setup #
#############

if (${SUSHI_WITH_LV2})
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

if (${SUSHI_WITH_RPC_INTERFACE})
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

if (${SUSHI_WITH_LINK})
    message("Building with Ableton Link support.")

    include(third-party/link/AbletonLinkConfig.cmake)

    # Linked libraries
    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} Ableton::Link)

    # Compiler definitions
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_ABLETON_LINK)
endif()

message("Configured audio buffer size: " ${SUSHI_AUDIO_BUFFER_SIZE} " samples")

###################
# sentry.io setup #
###################

if (${SUSHI_WITH_SENTRY})
    find_package(Threads)
    find_package(sentry CONFIG REQUIRED)

    message("Building with sentry.io support.")

    if (NOT SUSHI_SENTRY_DSN)
        set(SUSHI_SENTRY_DSN ${SUSHI_SENTRY_DSN_DEFAULT} CACHE STRING "The DSN to upload the sentry logs and crash reports to, from Sushi" FORCE)
    endif()

    set(SENTRY_CRASHPAD_HANDLER_PATH "${VCPKG_INSTALLED_DIR}/osx-custom/tools/sentry-native/crashpad_handler")

    set(EXTRA_BUILD_LIBRARIES ${EXTRA_BUILD_LIBRARIES} sentry::sentry)
    set(EXTRA_COMPILE_DEFINITIONS ${EXTRA_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_SENTRY -DSUSHI_SENTRY_DSN="${SUSHI_SENTRY_DSN}")
endif()

####################
#  Main Target     #
####################

set(SOURCE_FILES
    src/utils.cpp
    src/audio_frontends/base_audio_frontend.cpp
    src/concrete_sushi.cpp
    src/audio_frontends/offline_frontend.cpp
    src/audio_frontends/reactive_frontend.cpp
    src/audio_frontends/jack_frontend.cpp
    src/audio_frontends/portaudio_frontend.cpp
    src/audio_frontends/apple_coreaudio_frontend.cpp
    src/audio_frontends/portaudio_devices_dump.cpp
    src/audio_frontends/coreaudio_devices_dump.cpp
    src/audio_frontends/xenomai_raspa_frontend.cpp
    src/audio_frontends/offline_frontend.cpp
    src/audio_frontends/reactive_frontend.cpp
    src/control_frontends/base_control_frontend.cpp
    src/control_frontends/oscpack_osc_messenger.cpp
    src/control_frontends/reactive_midi_frontend.cpp
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
    src/engine/plugin_library.cpp
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
    src/engine/controller/real_time_controller.cpp
    src/factories/base_factory.cpp
    src/factories/reactive_factory.cpp
    src/factories/reactive_factory_implementation.cpp
    src/factories/standalone_factory.cpp
    src/factories/standalone_factory_implementation.cpp
    src/factories/offline_factory.cpp
    src/factories/offline_factory_implementation.cpp
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
    src/plugins/freeverb_plugin.cpp
    src/plugins/peak_meter_plugin.cpp
    src/plugins/return_plugin.cpp
    src/plugins/sample_player_plugin.cpp
    src/plugins/sample_player_voice.cpp
    src/plugins/sample_delay_plugin.cpp
    src/plugins/send_plugin.cpp
    src/plugins/send_return_factory.cpp
    src/plugins/step_sequencer_plugin.cpp
    src/plugins/transposer_plugin.cpp
    src/plugins/wav_streamer_plugin.cpp
    src/plugins/wav_writer_plugin.cpp
    src/plugins/mono_summing_plugin.cpp
    src/plugins/stereo_mixer_plugin.cpp
    src/plugins/brickworks/compressor_plugin.cpp
    src/plugins/brickworks/bitcrusher_plugin.cpp
    src/plugins/brickworks/wah_plugin.cpp
    src/plugins/brickworks/eq3band_plugin.cpp
    src/plugins/brickworks/phaser_plugin.cpp
    src/plugins/brickworks/chorus_plugin.cpp
    src/plugins/brickworks/vibrato_plugin.cpp
    src/plugins/brickworks/flanger_plugin.cpp
    src/plugins/brickworks/combdelay_plugin.cpp
    src/plugins/brickworks/saturation_plugin.cpp
    src/plugins/brickworks/noise_gate_plugin.cpp
    src/plugins/brickworks/tremolo_plugin.cpp
    src/plugins/brickworks/notch_plugin.cpp
    src/plugins/brickworks/multi_filter_plugin.cpp
    src/plugins/brickworks/highpass_plugin.cpp
    src/plugins/brickworks/clip_plugin.cpp
    src/plugins/brickworks/fuzz_plugin.cpp
    src/plugins/brickworks/dist_plugin.cpp
    src/plugins/brickworks/drive_plugin.cpp
    src/plugins/brickworks/simple_synth_plugin.cpp
)

add_library(${PROJECT_NAME} STATIC "${SOURCE_FILES}"
                                   "${ADDITIONAL_APPLE_COREAUDIO_SOURCES}"
                                   "${ADDITIONAL_VST2_SOURCES}"
                                   "${ADDITIONAL_VST3_SOURCES}"
                                   "${ADDITIONAL_LV2_SOURCES}"
                                   "${MIDI_SOURCES}")

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

#########################
#  Include Directories  #
#########################

set(INCLUDE_DIRS ${INCLUDE_DIRS}
                 "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}"
                 "${PROJECT_SOURCE_DIR}/third-party/vst3sdk"
                 "${PROJECT_SOURCE_DIR}/third-party/link/include"
                 "${PROJECT_SOURCE_DIR}/third-party/brickworks/include"
)


set(PUBLIC_INCLUDE_DIRS "${CMAKE_BINARY_DIR}" # for generated version.h
                        "${PROJECT_SOURCE_DIR}/third-party/optionparser/"
                        "${PROJECT_SOURCE_DIR}/third-party/rapidjson/include")

#################################
#  Linked libraries             #
#################################

find_package(SndFile CONFIG REQUIRED)

set(COMMON_LIBRARIES
    SndFile::sndfile
    pthread
    dl
    fifo
    oscpack
    elklog
    freeverb
    ${TWINE_LIB}
)

set_target_properties(oscpack PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(COMMON_LIBRARIES ${COMMON_LIBRARIES} atomic)
endif()

target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${INCLUDE_DIRS}
        PUBLIC
            ${PUBLIC_INCLUDE_DIRS}
            # where top-level project will look for the library's public headers
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${EXTRA_BUILD_LIBRARIES} ${COMMON_LIBRARIES} elk::warningSuppressor)

####################################
#  Compiler Flags and definitions  #
####################################

target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Wno-psabi -fPIC -ffast-math)

target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti)

if (SUSHI_BUILD_WITH_SANITIZERS)
    target_compile_options(${PROJECT_NAME}  PUBLIC -fsanitize=address -g)
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_definitions(${PROJECT_NAME} PUBLIC -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=${SUSHI_AUDIO_BUFFER_SIZE} ${EXTRA_COMPILE_DEFINITIONS})

#####################
#  Sub projects     #
#####################

if (${SUSHI_BUILD_STANDALONE_APP})
    add_subdirectory(apps)
endif()

######################
#  Tests subproject  #
######################

if (${SUSHI_WITH_UNIT_TESTS})
    add_subdirectory(test)
endif()
