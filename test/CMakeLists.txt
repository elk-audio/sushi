#####################################
#  Again Plugin                     #
#####################################

# Build Again example plugin from VsT SDK, needed for dynamic library loading tests
# Adapted from https://github.com/gmoe/vst-cmake

# API fix for Linux builds
if(UNIX AND NOT APPLE)
    add_definitions( -D__cdecl= )
endif()

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

include_directories(${VST_SDK_PATH})

add_library(again MODULE ${AGAIN_SOURCES})
# Suppress VST SDK warnings
set_target_properties(again PROPERTIES
        COMPILE_FLAGS "-Wno-unused -Wno-multichar -Wno-write-strings -Wno-narrowing"
    )
if(APPLE)
set_target_properties(again PROPERTIES
        BUNDLE true
        BUNDLE_EXTENSION "vst"
        XCODE_ATTRIBUTE_WRAPPER_EXTENSION "vst"
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/test/resources/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "AGain"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.Testing.AGain"
        MACOSX_BUNDLE_ICON_FILE ""
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_COPYRIGHT "Again Testing Plugin © 2016"
    )
endif()

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
               unittests/audio_frontends/offline_frontend_test.cpp
               unittests/library/sample_buffer_test.cpp
               unittests/library/plugin_parameters_test.cpp
               unittests/library/internal_plugin_test.cpp
               unittests/library/plugin_events_test.cpp
        )

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

