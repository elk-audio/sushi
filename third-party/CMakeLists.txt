cmake_minimum_required(VERSION 3.8)

add_subdirectory(fifo EXCLUDE_FROM_ALL)

##########################
#  VST3SDK and examples  #
##########################

# Exclude VstGui, hosting samples and skip validation
set(SMTG_ADD_VSTGUI OFF CACHE BOOL "" FORCE)
set(SMTG_ADD_VST3_PLUGINS_SAMPLES ON CACHE BOOL "" FORCE)
set(SMTG_ADD_VST3_HOSTING_SAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)

# Don't install example plugins in ~/.vst3, keep them in the build directory
set(SMTG_PLUGIN_TARGET_PATH ${CMAKE_CURRENT_BINARY_DIR}/vst3sdk/VST3 CACHE STRING "" FORCE)
set(SMTG_LINUX ON)

add_subdirectory(vst3sdk)

# Supress warnings to keep our build output clean
set_target_properties(base PROPERTIES COMPILE_FLAGS "-Wno-deprecated -Wno-cpp")


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