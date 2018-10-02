cmake_minimum_required(VERSION 3.8)

add_subdirectory(EASTL EXCLUDE_FROM_ALL)
add_subdirectory(vstsdk3 EXCLUDE_FROM_ALL)
add_subdirectory(fifo EXCLUDE_FROM_ALL)

#####################################################
#  External Projects definitions (non-Cmake based)  #
#####################################################

# Add here custom build steps for third-party libraries
# that need to be linked statically,
# example:
#
# include(ExternalProject)
#
# ExternalProject_Add(static_library
#         SOURCE_DIR ${PROJECT_SOURCE_DIR}/third-party/static_library
#         BUILD_IN_SOURCE 1
#         CONFIGURE_COMMAND ./autogen.sh COMMAND ./configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/install/static_library
#         BUILD_COMMAND make
#         INSTALL_COMMAND make install
# )