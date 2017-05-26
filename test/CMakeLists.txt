#####################################
#  Again Plugin                     #
#####################################

# Build Again example plugin from VsT SDK, needed for dynamic library loading tests
# Adapted from https://github.com/gmoe/vst-cmake


set(VST_SDK_PLUGIN_SOURCES
        "${VST_SDK_PATH}/public.sdk/source/vst2.x/audioeffectx.cpp"
        "${VST_SDK_PATH}/public.sdk/source/vst2.x/audioeffect.cpp"
        "${VST_SDK_PATH}/public.sdk/source/vst2.x/vstplugmain.cpp"
        "${VST_SDK_PATH}/pluginterfaces/vst2.x/aeffectx.h"
    )

set(AGAIN_SOURCE_DIR "${VST_SDK_PATH}/public.sdk/samples/vst2.x/again/source")
set(AGAIN_SOURCES
        "${AGAIN_SOURCE_DIR}/again.h"
        "${AGAIN_SOURCE_DIR}/again.cpp"
        "${VST_SDK_PLUGIN_SOURCES}"
    )

set(VSTXSYNTH_SOURCE_DIR "${VST_SDK_PATH}/public.sdk/samples/vst2.x/vstxsynth/source")
set(VSTXSYNTH_SOURCES
        "${VSTXSYNTH_SOURCE_DIR}/vstxsynth.cpp"
        "${VSTXSYNTH_SOURCE_DIR}/vstxsynthproc.cpp"
        "${VST_SDK_PLUGIN_SOURCES}"
        )

include_directories(${VST_SDK_PATH})

add_library(again MODULE ${AGAIN_SOURCES})
# Suppress VST SDK warnings
set_target_properties(again PROPERTIES
        COMPILE_FLAGS "-Wno-unused -Wno-multichar -Wno-write-strings -Wno-narrowing"
    )

add_library(vstxsynth MODULE ${VSTXSYNTH_SOURCES})
# Suppress VST SDK warnings
set_target_properties(vstxsynth PROPERTIES
        COMPILE_FLAGS "-Wno-unused -Wno-sequence-point -Wno-multichar -Wno-write-strings -Wno-narrowing"
        )

#####################################
#  Unit Tests Targets               #
#####################################

# build gtest framework
add_subdirectory(gtest_1_7_0)
enable_testing()

#####################################
#  Disable logging at compile time  #
#####################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_LOGGING")

#####################
#  Unit Test Files  #
#####################

SET(TEST_FILES unittests/sample_test.cpp
               unittests/plugins/plugins_test.cpp
               unittests/plugins/sample_player_plugin_test.cpp
               unittests/engine/plugin_chain_test.cpp
               unittests/engine/engine_test.cpp
               unittests/engine/midi_dispatcher_test.cpp
               unittests/engine/json_configurator_test.cpp
               unittests/audio_frontends/offline_frontend_test.cpp
               unittests/control_frontends/osc_frontend_test.cpp
               unittests/dsp_library/envelope_test.cpp
               unittests/dsp_library/sample_wrapper_test.cpp
               unittests/library/sample_buffer_test.cpp
               unittests/library/midi_decoder_test.cpp
               unittests/library/plugin_parameters_test.cpp
               unittests/library/internal_plugin_test.cpp
               unittests/library/plugin_events_test.cpp
               unittests/library/vst2x_plugin_loader_test.cpp
               unittests/library/vst2x_wrapper_test.cpp
               unittests/library/vst2x_midi_event_fifo_test.cpp
               unittests/library/id_generator_test.cpp)

if (${WITH_JACK})
    set(TEST_FILES ${TEST_FILES} unittests/audio_frontends/jack_frontend_test.cpp)
endif()

#################################################
#  Additional compilation units needed by tests #
#################################################

SET(TEST_HELPER_FILES ${PROJECT_SOURCE_DIR}/src/library/vst2x_host_callback.cpp)

##########################################
#  Include directories relative to test  #
##########################################

set(INCLUDE_DIRS "${INCLUDE_DIRS}"
                 ${PROJECT_SOURCE_DIR}/test/unittests
                 ${PROJECT_SOURCE_DIR}/test/gtest_1_7_0/include)

include_directories(${INCLUDE_DIRS})

#################################
#  Statically linked libraries  #
#################################

set(LINK_LIBRARIES "${LINK_LIBRARIES}"
                   gtest
                   gtest_main)

add_executable(unit_tests ${TEST_FILES} ${TEST_HELPER_FILES})

target_link_libraries(unit_tests "${LINK_LIBRARIES}")
add_test(unit_tests unit_tests)
add_dependencies(unit_tests again vstxsynth)

### Custom target for running the tests
# Environment variable pointing to test/data/ is set so that
# tests can read it to access data files maintaining an independent out-of-source build

add_custom_target(run_tests ALL
                  ${CMAKE_COMMAND}
                  -E env "SUSHI_TEST_DATA_DIR=${PROJECT_SOURCE_DIR}/test/data"
                  "./unit_tests")
add_dependencies(run_tests unit_tests)

