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

SET(TEST_FILES unittests/sample_test.cpp)

##########################################
#  Include directories relative to test  #
##########################################

set(INCLUDE_DIRS "${INCLUDE_DIRS}"
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

# custom target for running the tests
add_custom_target(run_tests ALL COMMAND "./unit_tests")
add_dependencies(run_tests unit_tests)

