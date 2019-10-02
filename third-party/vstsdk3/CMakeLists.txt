
cmake_minimum_required (VERSION 3.4.3)
#-------------------------------------------------------------------------------
# Adapted from CMakeLists.txt in vst3sdk but edited to remove dependencies
# vstgui4 which is horribly bloated and requires massive dependencies (1GB
# downloads).
#-------------------------------------------------------------------------------


#-------------------------------------------------------------------------------
# Includes
#-------------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(Global)
include(AddVST3Library)
include(Bundle)
include(ExportedSymbols)
include(PrefixHeader)
include(PlatformToolset)

#-------------------------------------------------------------------------------
# Custom tweaking for MIND
#-------------------------------------------------------------------------------

option(SMTG_LINUX "Linux is assumed" ON)
set(WARNINGS_DISABLED "-w")

#-------------------------------------------------------------------------------
# SDK Project
#-------------------------------------------------------------------------------
project(vstsdk)

setupPlatformToolset()

set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Here you can define where the VST 3 SDK is located
set(SDK_ROOT "${ROOT}")
set(public_sdk_SOURCE_DIR ${SDK_ROOT}/public.sdk)
set(pluginterfaces_SOURCE_DIR ${SDK_ROOT}/pluginterfaces)

include_directories(${ROOT} ${SDK_ROOT})

#-------------------------------------------------------------------------------
# Projects
#-------------------------------------------------------------------------------
set(SDK_IDE_LIBS_FOLDER FOLDER "Libraries")

#---Add base libraries---------------------------
set(VST_SDK TRUE) # used for Base module which provides only a subset of Base for VST-SDK
add_subdirectory(base)
add_subdirectory(public.sdk)
add_subdirectory(public.sdk/source/vst/interappaudio)

#-------------------------------------------------------------------------------
# Here is added your own plugins folder location
#-------------------------------------------------------------------------------

#---Add VST3 examples (plug-ins and hosting)-----
set(SDK_IDE_PLUGIN_EXAMPLES_FOLDER FOLDER "PlugInExamples")
add_subdirectory(public.sdk/samples/vst/adelay)
add_subdirectory(public.sdk/samples/vst/mda-vst3)

# Add hosting examples, it includes the validator
set(SDK_IDE_HOSTING_EXAMPLES_FOLDER FOLDER "HostingExamples")
add_subdirectory(public.sdk/samples/vst-hosting/validator)

#-------------------------------------------------------------------------------
# IDE sorting
#-------------------------------------------------------------------------------

set_target_properties(sdk PROPERTIES ${SDK_IDE_LIBS_FOLDER})
set_target_properties(base PROPERTIES ${SDK_IDE_LIBS_FOLDER})

#-------------------------------------------------------------------------------
# Silence warnings from Steinberg Code
#-------------------------------------------------------------------------------

set_target_properties(adelay PROPERTIES COMPILE_FLAGS ${WARNINGS_DISABLED})
set_target_properties(sdk PROPERTIES COMPILE_FLAGS ${WARNINGS_DISABLED})
set_target_properties(base PROPERTIES COMPILE_FLAGS ${WARNINGS_DISABLED})
set_target_properties(validator PROPERTIES COMPILE_FLAGS ${WARNINGS_DISABLED})
