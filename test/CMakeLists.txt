######################
#  Vst test plugins  #
######################

# Build Test plugin, needed for dynamic library loading unit tests
# Adapted from https://github.com/gmoe/vst-cmake

if (${WITH_VST2})
    set(VST2_SDK_PLUGIN_SOURCES
            "${VST2_SDK_PATH}/public.sdk/source/vst2.x/audioeffectx.cpp"
            "${VST2_SDK_PATH}/public.sdk/source/vst2.x/audioeffect.cpp"
            "${VST2_SDK_PATH}/public.sdk/source/vst2.x/vstplugmain.cpp"
            "${VST2_SDK_PATH}/pluginterfaces/vst2.x/aeffectx.h"
        )

    set(VST2_TEST_PLUGIN_SOURCES
            unittests/test_utils/vst2_test_plugin.h
            unittests/test_utils/vst2_test_plugin.cpp
            ${VST2_SDK_PLUGIN_SOURCES}
        )

    add_library(vst2_test_plugin MODULE ${VST2_TEST_PLUGIN_SOURCES})
    # Suppress VST SDK warnings
    set(VST2_COMPILE_FLAS -Wall -Wno-write-strings -Wno-narrowing)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(VST2_COMPILE_FLAS ${VST2_COMPILE_FLAGS} -Wno-stringop-truncation)
    endif()
    set_target_properties(vst2_test_plugin PROPERTIES
            COMPILE_FLAGS "${VST2_COMPILE_FLAGS}"
        )
    target_include_directories(vst2_test_plugin PRIVATE ${VST2_SDK_PATH})
    target_compile_definitions(vst2_test_plugin PRIVATE -D__cdecl= )
endif()

#####################################
#  Unit Tests Targets               #
#####################################

# build gtest framework
add_subdirectory(gtest)
enable_testing()

#####################
#  Unit Test Files  #
#####################

SET(TEST_FILES unittests/sample_test.cpp
               unittests/plugins/arpeggiator_plugin_test.cpp
               unittests/plugins/control_to_cv_plugin_test.cpp
               unittests/plugins/cv_to_control_plugin_test.cpp
               unittests/plugins/plugins_test.cpp
               unittests/plugins/sample_player_plugin_test.cpp
               unittests/plugins/send_return_test.cpp
               unittests/plugins/step_sequencer_test.cpp
               unittests/engine/audio_graph_test.cpp
               unittests/engine/track_test.cpp
               unittests/engine/engine_test.cpp
               unittests/engine/processor_container_test.cpp
               unittests/engine/midi_dispatcher_test.cpp
               unittests/engine/json_configurator_test.cpp
               unittests/engine/receiver_test.cpp
               unittests/engine/event_dispatcher_test.cpp
               unittests/engine/event_timer_test.cpp
               unittests/engine/transport_test.cpp
               unittests/engine/controller_test.cpp
               unittests/engine/controllers/audio_graph_controller_test.cpp
               unittests/engine/controllers/audio_routing_controller_test.cpp
               unittests/engine/controllers/osc_controller_test.cpp
               unittests/engine/controllers/midi_controller_test.cpp
               unittests/audio_frontends/offline_frontend_test.cpp
               unittests/control_frontends/osc_frontend_test.cpp
               unittests/dsp_library/envelope_test.cpp
               unittests/dsp_library/master_limiter_test.cpp
               unittests/dsp_library/sample_wrapper_test.cpp
               unittests/dsp_library/value_smoother_test.cpp
               unittests/library/event_test.cpp
               unittests/library/processor_test.cpp
               unittests/library/sample_buffer_test.cpp
               unittests/library/midi_decoder_test.cpp
               unittests/library/midi_encoder_test.cpp
               unittests/library/parameter_dump_test.cpp
               unittests/library/performance_timer_test.cpp
               unittests/library/plugin_parameters_test.cpp
               unittests/library/internal_plugin_test.cpp
               unittests/library/rt_event_test.cpp
               unittests/library/id_generator_test.cpp
               unittests/library/simple_fifo_test.cpp)

if (${WITH_JACK})
    set(TEST_FILES ${TEST_FILES} unittests/audio_frontends/jack_frontend_test.cpp)
endif()

if (${WITH_VST2})
    set(TEST_FILES ${TEST_FILES} unittests/library/vst2x_wrapper_test.cpp
                                 unittests/library/vst2x_plugin_loading_test.cpp
                                 unittests/library/vst2x_midi_event_fifo_test.cpp
                                 ${PROJECT_SOURCE_DIR}/src/library/vst2x/vst2x_host_callback.cpp)
endif()

if (${WITH_VST3})
    set(TEST_FILES ${TEST_FILES} unittests/library/vst3x_wrapper_test.cpp)
endif()

set(TEST_HELPER_FILES ${TEST_HELPER_FILES} ${PROJECT_SOURCE_DIR}/src/plugins/transposer_plugin.cpp
                                           ${PROJECT_SOURCE_DIR}/src/library/lv2/lv2_processor_factory.cpp
                                           ${PROJECT_SOURCE_DIR}/src/library/vst2x/vst2x_processor_factory.cpp
                                           ${PROJECT_SOURCE_DIR}/src/library/vst3x/vst3x_processor_factory.cpp)

if (${WITH_LV2})
    set(TEST_FILES ${TEST_FILES} unittests/library/lv2_wrapper_test.cpp)
endif()

add_executable(unit_tests ${TEST_FILES} ${TEST_HELPER_FILES})

target_compile_definitions(unit_tests PRIVATE -DSUSHI_DISABLE_LOGGING
                                              -D__cdecl=
                                              -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=${AUDIO_BUFFER_SIZE})

target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wno-psabi -fno-rtti -ffast-math)

if (${WITH_JACK})
    target_compile_definitions(unit_tests PRIVATE -DSUSHI_BUILD_WITH_JACK)
endif()

if (${WITH_VST2})
    target_compile_definitions(unit_tests PRIVATE -DSUSHI_BUILD_WITH_VST2)
endif()

if (${WITH_VST3})
    target_compile_definitions(unit_tests PRIVATE -DSUSHI_BUILD_WITH_VST3)
endif()

if (${WITH_LV2})
    target_compile_definitions(unit_tests PRIVATE -DSUSHI_BUILD_WITH_LV2)
endif()

if (${WITH_LV2_MDA_TESTS})
    target_compile_definitions(unit_tests PRIVATE -DSUSHI_BUILD_WITH_LV2_MDA_TESTS)
endif()

##########################################
#  Include directories relative to test  #
##########################################

set(INCLUDE_DIRS "${INCLUDE_DIRS}"
                 ${PROJECT_SOURCE_DIR}/test/unittests
                 ${PROJECT_SOURCE_DIR}/test/gtest/include)

target_include_directories(unit_tests PRIVATE ${INCLUDE_DIRS})

#################################
#  Statically linked libraries  #
#################################

set(TEST_LINK_LIBRARIES
    ${COMMON_LIBRARIES}
    gtest
    gtest_main
)

if (${WITH_VST2})
    add_dependencies(unit_tests vst2_test_plugin)
endif()

if (${WITH_VST3})
    set(TEST_LINK_LIBRARIES ${TEST_LINK_LIBRARIES} vst3_host sdk base)
    add_dependencies(unit_tests adelay vst3_host)
endif()

if (${WITH_JACK})
    set(TEST_LINK_LIBRARIES ${TEST_LINK_LIBRARIES})# asound)
endif()

if (${WITH_LV2})
    set(TEST_LINK_LIBRARIES ${TEST_LINK_LIBRARIES} asound lilv-0 lv2_host)
    add_dependencies(unit_tests lv2_host)
endif()

# Disable Link in tests as this slows down the testing a lot due to network activity etc
remove_definitions(-DSUSHI_BUILD_WITH_ABLETON_LINK)

target_link_libraries(unit_tests "${TEST_LINK_LIBRARIES}")
add_test(unit_tests unit_tests)

### Custom target for running the tests
# Environment variable pointing to test/data/ is set so that
# tests can read it to access data files maintaining an independent out-of-source build

add_custom_target(run_tests ALL
                  ${CMAKE_COMMAND}
                  -E env "SUSHI_TEST_DATA_DIR=${PROJECT_SOURCE_DIR}/test/data"
                  "./unit_tests")
add_dependencies(run_tests unit_tests)
