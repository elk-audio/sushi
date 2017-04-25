# build gtest framework
add_subdirectory(gtest_1_7_0)
enable_testing()

#####################################
#  Disable logging at compile time  #
#####################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_LOGGING")

################
#  Test Files  #
################

SET(TEST_FILES unittests/sample_test.cpp
               unittests/plugins/plugins_test.cpp
               unittests/plugins/sample_player_plugin_test.cpp
               unittests/engine/plugin_chain_test.cpp
               unittests/engine/engine_test.cpp
               unittests/engine/midi_dispatcher_test.cpp
               unittests/audio_frontends/offline_frontend_test.cpp
               unittests/audio_frontends/jack_frontend_test.cpp
               unittests/control_frontends/osc_frontend_test.cpp
               unittests/dsp_library/envelope_test.cpp
               unittests/dsp_library/sample_wrapper_test.cpp
               unittests/library/sample_buffer_test.cpp
               unittests/library/midi_decoder_test.cpp
               unittests/library/plugin_parameters_test.cpp
               unittests/library/internal_plugin_test.cpp
               unittests/library/plugin_events_test.cpp)

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

add_executable(unit_tests ${TEST_FILES})

target_link_libraries(unit_tests "${LINK_LIBRARIES}")
add_test(unit_tests unit_tests)

### Custom target for running the tests
# Environment variable pointing to test/data/ is set so that
# tests can read it to access data files maintaining an independent out-of-source build

add_custom_target(run_tests ALL
                  ${CMAKE_COMMAND}
                  -E env "SUSHI_TEST_DATA_DIR=${PROJECT_SOURCE_DIR}/test/data"
                  "./unit_tests")
add_dependencies(run_tests unit_tests)

