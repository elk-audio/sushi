cmake_minimum_required(VERSION 2.8)
project(sushi)

####################
#  Compiler Flags  #
####################

# Global flags
set(PROJECT_CXX_FLAGS "-Wall -Wextra -std=c++14 -fno-rtti")
set(PROJECT_LINK_FLAGS "")

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
                      src/logging.cpp)


# Enumerate all the headers separately so that CLion can index them

set(EXTRA_CLION_SOURCES src/logging.h
                        src/options.h)

set(SOURCE_FILES "${COMPILATION_UNITS}" "${EXTRA_CLION_SOURCES}")

#########################
#  Include Directories  #
#########################

set(INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src"
                 "${PROJECT_SOURCE_DIR}/include"
                 "${PROJECT_SOURCE_DIR}/third-party/optionparser/"
                 "${PROJECT_SOURCE_DIR}/third-party")

# /usr/local doesn't get added by default in Mac OS X
if (APPLE)
    set(INCLUDE_DIRS "${INCLUDE_DIRS}" /usr/local/include)
endif()

include_directories(${INCLUDE_DIRS})

#################################
#  Statically linked libraries  #
#################################

# Eventually define here the CMake var
# LINK_LIBRARIES
# to point to statically linked libraries,
# example:

# set(LINK_LIBRARIES "${PROJECT_SOURCE_DIR}/third-party/install/static_library/lib/libstaticlibrary.a"
#                    pthread)
 
if (APPLE)
    link_directories(/usr/local/lib)
endif()

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
# target_link_libraries(sushi "${LINK_LIBRARIES}")
