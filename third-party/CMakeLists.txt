cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

#####################################################
#  External Projects definitions (non-Cmake based)  #
#####################################################

# Add here custom build steps for third-party libraries
# that need to be linked statically,
# example:

# ExternalProject_Add(static_library
#         SOURCE_DIR ${PROJECT_SOURCE_DIR}/third-party/static_library
#         BUILD_IN_SOURCE 1
#         CONFIGURE_COMMAND ./autogen.sh COMMAND ./configure --prefix=${CMAKE_CURRENT_SOURCE_DIR}/install/static_library
#         BUILD_COMMAND make
#         INSTALL_COMMAND make install
# )

