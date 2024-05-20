#####################################
#  Unit Tests Targets               #
#####################################

# build gtest framework
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(gtest)
enable_testing()

#####################
#  Unit Test Files  #
#####################

SET(TEST_FILES
    unittests/sample_test.cpp
    unittests/plugins/arpeggiator_plugin_test.cpp
    unittests/plugins/control_to_cv_plugin_test.cpp
    unittests/plugins/cv_to_control_plugin_test.cpp
    unittests/plugins/plugins_test.cpp
    unittests/plugins/external_plugins_test.cpp
    unittests/plugins/brickworks_simple_synth_test.cpp
    unittests/plugins/sample_player_plugin_test.cpp
    unittests/plugins/send_return_test.cpp
    unittests/plugins/step_sequencer_test.cpp
    unittests/plugins/wav_streamer_plugin_test.cpp
    unittests/engine/audio_graph_test.cpp
    unittests/engine/track_test.cpp
    unittests/engine/engine_test.cpp
    unittests/engine/parameter_manager_test.cpp
    unittests/engine/processor_container_test.cpp
    unittests/engine/midi_dispatcher_test.cpp
    unittests/engine/json_configurator_test.cpp
    unittests/engine/receiver_test.cpp
    unittests/engine/event_dispatcher_test.cpp
    unittests/engine/event_timer_test.cpp
    unittests/engine/transport_test.cpp
    unittests/engine/controller_test.cpp
    unittests/engine/plugin_library_test.cpp
    unittests/engine/controllers/audio_graph_controller_test.cpp
    unittests/engine/controllers/audio_routing_controller_test.cpp
    unittests/engine/controllers/osc_controller_test.cpp
    unittests/engine/controllers/midi_controller_test.cpp
    unittests/engine/controllers/session_controller_test.cpp
    unittests/engine/controllers/reactive_controller_test.cpp
    unittests/engine/factories/factories_test.cpp
    unittests/audio_frontends/offline_frontend_test.cpp
    unittests/control_frontends/osc_frontend_test.cpp
    unittests/control_frontends/oscpack_osc_messenger_test.cpp
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
    unittests/library/simple_fifo_test.cpp
    unittests/library/fixed_stack_test.cpp
)

set(TEST_HELPER_FILES ${TEST_HELPER_FILES}
    ${PROJECT_SOURCE_DIR}/src/library/processor_state.cpp
    ${PROJECT_SOURCE_DIR}/src/audio_frontends/base_audio_frontend.cpp
    ${PROJECT_SOURCE_DIR}/src/plugins/transposer_plugin.cpp
    ${PROJECT_SOURCE_DIR}/src/library/lv2/lv2_processor_factory.cpp
    ${PROJECT_SOURCE_DIR}/src/library/vst2x/vst2x_processor_factory.cpp
    ${PROJECT_SOURCE_DIR}/src/library/vst3x/vst3x_processor_factory.cpp
    ${PROJECT_SOURCE_DIR}/test/unittests/test_utils/portaudio_mockup.cpp
    ${FREEVERB_SOURCES}
)

set(INCLUDE_DIRS ${INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/test/unittests
    ${PROJECT_SOURCE_DIR}/test/gtest/include
)

set(TEST_LINK_LIBRARIES
    ${COMMON_LIBRARIES}
    gtest
    gtest_main
    gmock
    gmock_main
)

set(TEST_COMPILE_DEFINITIONS
    -DELKLOG_DISABLE_LOGGING
    -D__cdecl=
    -DSUSHI_CUSTOM_AUDIO_CHUNK_SIZE=${SUSHI_AUDIO_BUFFER_SIZE}
)

set(TEST_DEPENDENCIES "")


######################
# Build options      #
######################

if (APPLE)
    set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DSUSHI_APPLE_THREADING)
endif()

# Build Test plugin, needed for dynamic library loading unit tests
# Adapted from https://github.com/gmoe/vst-cmake

if (${SUSHI_WITH_VST2})
    set(VST2_SDK_PLUGIN_SOURCES
            "${SUSHI_VST2_SDK_PATH}/public.sdk/source/vst2.x/audioeffectx.cpp"
            "${SUSHI_VST2_SDK_PATH}/public.sdk/source/vst2.x/audioeffect.cpp"
            "${SUSHI_VST2_SDK_PATH}/public.sdk/source/vst2.x/vstplugmain.cpp"
            "${SUSHI_VST2_SDK_PATH}/pluginterfaces/vst2.x/aeffectx.h"
    )

    set(VST2_TEST_PLUGIN_SOURCES
            unittests/test_utils/vst2_test_plugin.h
            unittests/test_utils/vst2_test_plugin.cpp
            ${VST2_SDK_PLUGIN_SOURCES}
    )
    if (MSVC)
        set(VST2_TEST_PLUGIN_SOURCES
                unittests/test_utils/vst2_test_plugin.def
                ${VST2_TEST_PLUGIN_SOURCES}
        )
    endif ()

    add_library(vst2_test_plugin MODULE ${VST2_TEST_PLUGIN_SOURCES})

    # Suppress VST SDK warnings
    set(VST2_COMPILE_FLAS -Wall -Wno-write-strings -Wno-narrowing)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(VST2_COMPILE_FLAS ${VST2_COMPILE_FLAGS} -Wno-stringop-truncation)
    endif()
    set_target_properties(vst2_test_plugin PROPERTIES
            COMPILE_FLAGS "${VST2_COMPILE_FLAGS}"
    )
    target_compile_features(vst2_test_plugin PUBLIC cxx_std_20)
    target_include_directories(vst2_test_plugin PRIVATE ${SUSHI_VST2_SDK_PATH})
    target_compile_definitions(vst2_test_plugin PRIVATE -D__cdecl= )
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        set_target_properties(vst2_test_plugin PROPERTIES
            BUNDLE true
            BUNDLE_EXTENSION "vst"
            XCODE_ATTRIBUTE_WRAPPER_EXTENSION "vst"
            MACOSX_BUNDLE_BUNDLE_NAME "TestPlugin"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.ELK.TestPlugin"
            MACOSX_BUNDLE_ICON_FILE ""
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
            MACOSX_BUNDLE_COPYRIGHT "ELK © 2016"
    )
    endif()

    set(TEST_FILES ${TEST_FILES}
        unittests/library/vst2x_wrapper_test.cpp
        unittests/library/vst2x_plugin_loading_test.cpp
        unittests/library/vst2x_midi_event_fifo_test.cpp
        ${PROJECT_SOURCE_DIR}/src/library/vst2x/vst2x_host_callback.cpp
    )

    set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_VST2)
    if (APPLE)
        set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DVST2_TEST_PLUGIN_PATH="$<TARGET_BUNDLE_DIR:vst2_test_plugin>")
        set(TEST_LINK_LIBRARIES ${TEST_LINK_LIBRARIES} "-framework CoreFoundation")
    else()
        set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DVST2_TEST_PLUGIN_PATH="$<TARGET_FILE:vst2_test_plugin>")
    endif()
    set(TEST_DEPENDENCIES ${TEST_DEPENDENCIES} vst2_test_plugin)
endif()

if (${SUSHI_WITH_JACK})
    set(TEST_FILES ${TEST_FILES} unittests/audio_frontends/jack_frontend_test.cpp)
    set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_JACK)
endif()

if (${SUSHI_WITH_PORTAUDIO})
    set(TEST_FILES ${TEST_FILES} unittests/audio_frontends/portaudio_frontend_test.cpp)
    set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_PORTAUDIO)
endif()

if (${SUSHI_WITH_APPLE_COREAUDIO})
    set(TEST_FILES ${TEST_FILES}
            unittests/audio_frontends/apple_coreaudio_frontend_test.cpp
            unittests/test_utils/apple_coreaudio_mockup.cpp
            ../src/audio_frontends/apple_coreaudio/apple_coreaudio_object.cpp
            ../src/audio_frontends/apple_coreaudio/apple_coreaudio_device.mm
            ../src/audio_frontends/apple_coreaudio/apple_coreaudio_utils.cpp)
    set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_APPLE_COREAUDIO)
    set(TEST_LINK_LIBRARIES ${TEST_LINK_LIBRARIES} "-framework CoreAudio -framework Foundation")
endif()

if (${SUSHI_WITH_VST3})
    set(TEST_FILES ${TEST_FILES} unittests/library/vst3x_wrapper_test.cpp)
    set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_VST3)
    set(TEST_LINK_LIBRARIES ${TEST_LINK_LIBRARIES} vst3_host sdk base)
    set(TEST_DEPENDENCIES ${TEST_DEPENDENCIES} adelay vst3_host)
endif()

if (${SUSHI_WITH_LV2})
    set(TEST_FILES ${TEST_FILES} unittests/library/lv2_wrapper_test.cpp)
    set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_LV2)
    set(TEST_LINK_LIBRARIES ${TEST_LINK_LIBRARIES} asound lilv-0 lv2_host)
    set(TEST_DEPENDENCIES ${TEST_DEPENDENCIES} lv2_host)
endif()

if (${SUSHI_WITH_LV2_MDA_TESTS})
    set(TEST_COMPILE_DEFINITIONS ${TEST_COMPILE_DEFINITIONS} -DSUSHI_BUILD_WITH_LV2_MDA_TESTS)
endif()

if (${SUSHI_WITH_ALSA_MIDI})
    find_library(ALSA_LIB NAMES asound)
    set(TEST_LINK_LIBRARIES ${TEST_LINK_LIBRARIES} ${ALSA_LIB})
endif()

###############################
#  Test target configuration  #
###############################
add_executable(unit_tests ${TEST_FILES} ${TEST_HELPER_FILES})

target_compile_definitions(unit_tests PRIVATE ${TEST_COMPILE_DEFINITIONS})

if(MSVC)
    # C5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
    #        is disabled for all - Spectre is not a concern for Sushi I don't think.
    # C4996: Deprecated warning. This was flooding the terminal, I might remove the suppression from here eventually,
    #        and out into the code again.
    target_compile_options(unit_tests PRIVATE /W3 /wd5045 /wd4996)
else ()
    target_compile_options(unit_tests PRIVATE -Wall -fno-rtti -ffast-math)
endif()

target_include_directories(unit_tests
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            ${INCLUDE_DIRS}
            ${PUBLIC_INCLUDE_DIRS})

target_link_libraries(unit_tests "${TEST_LINK_LIBRARIES}" elk::warningSuppressor)
if (TEST_DEPENDENCIES)
    add_dependencies(unit_tests ${TEST_DEPENDENCIES})
endif()

# Disable Link in tests as this slows down the testing a lot due to network activity etc
remove_definitions(-DSUSHI_BUILD_WITH_ABLETON_LINK)


if(${SUSHI_DISABLE_MULTICORE_UNIT_TESTS})
    target_compile_definitions(unit_tests PRIVATE -DDISABLE_MULTICORE_UNIT_TESTS)
endif()

add_test(unit_tests unit_tests)

### Custom target for running the tests
# Environment variable pointing to test/data/ is set so that
# tests can read it to access data files maintaining an independent out-of-source build

# Execute tests if we are not cross-compiling
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Detected cross-compiling: skipping unit tests run.")
else()
    add_custom_target(run_tests ALL
                      ${CMAKE_COMMAND}
                      -E env "SUSHI_TEST_DATA_DIR=${PROJECT_SOURCE_DIR}/test/data"
                      "$<TARGET_FILE:unit_tests>")
    add_dependencies(run_tests unit_tests)
endif()

