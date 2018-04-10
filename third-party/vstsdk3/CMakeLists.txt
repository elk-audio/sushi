
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

# do not build VST2 by default
option(SMTG_CREATE_VST2_VERSION "Use VST2" OFF)

add_definitions(-DDEVELOPMENT)

set(WARNINGS_DISABLED "-w")
#-------------------------------------------------------------------------------
# SDK Project
#-------------------------------------------------------------------------------
set(VST_SDK TRUE)
project(vstsdk)

if (LINUX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    option(SMTG_ADD_ADDRESS_SANITIZER_CONFIG "Add AddressSanitizer Config (Linux only)" OFF)
	if(SMTG_ADD_ADDRESS_SANITIZER_CONFIG)
		set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES};ASan")
		add_compile_options($<$<CONFIG:ASan>:-DDEVELOPMENT=1>)
		add_compile_options($<$<CONFIG:ASan>:-fsanitize=address>)
		add_compile_options($<$<CONFIG:ASan>:-g>)
		add_compile_options($<$<CONFIG:ASan>:-O0>)
		set(ASAN_LIBRARY asan)
		link_libraries($<$<CONFIG:ASan>:${ASAN_LIBRARY}>)
	endif()
endif()

if(UNIX)
	if(XCODE)
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
		set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	elseif(APPLE)
		set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
		link_libraries(c++)
	else()
		set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-multichar")
		link_libraries(pthread dl)
	endif()
endif()

set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# here you can define where the VST3 SDK is located
set(SDK_ROOT "${ROOT}")

include_directories(${ROOT} ${SDK_ROOT})

set(SDK_IDE_LIBS_FOLDER FOLDER "Libraries")
set(SDK_IDE_PLUGIN_EXAMPLES_FOLDER FOLDER "PlugInExamples")
set(SDK_IDE_HOSTING_EXAMPLES_FOLDER FOLDER "HostingExamples")

#-------------------------------------------------------------------------------
if(MAC AND XCODE)
	if(NOT SMTG_COREAUDIO_SDK_PATH)
		# Check if the CoreAudio SDK is next to the VST3SDK:
		if(EXISTS "${SDK_ROOT}/../CoreAudio/AudioUnits/AUPublic/AUBase/AUBase.cpp")
			set(SMTG_COREAUDIO_SDK_PATH "${SDK_ROOT}/../CoreAudio")
		else()
			if(EXISTS "${SDK_ROOT}/external.apple.coreaudio/AudioUnits/AUPublic/AUBase/AUBase.cpp")
				set(SMTG_COREAUDIO_SDK_PATH "${SDK_ROOT}/external.apple.coreaudio")	
			endif()
		endif()
	else()
		if(NOT IS_ABSOLUTE ${SMTG_COREAUDIO_SDK_PATH})
			get_filename_component(SMTG_COREAUDIO_SDK_PATH "${SDK_ROOT}/${SMTG_COREAUDIO_SDK_PATH}" ABSOLUTE)
		endif()
		if(NOT EXISTS "${SMTG_COREAUDIO_SDK_PATH}/AudioUnits/AUPublic/AUBase/AUBase.cpp")
			message(FATAL_ERROR "SMTG_COREAUDIO_SDK_PATH is set but does not point to an expected location")
		endif()
	endif()
	if(SMTG_COREAUDIO_SDK_PATH)
		message(STATUS "SMTG_COREAUDIO_SDK_PATH is set to : " ${SMTG_COREAUDIO_SDK_PATH})
	endif()
endif()

#-------------------------------------------------------------------------------
# Projects
#-------------------------------------------------------------------------------

add_subdirectory(base)
add_subdirectory(public.sdk)
add_subdirectory(public.sdk/samples/vst/adelay)
add_subdirectory(public.sdk/samples/vst/mda-vst3)
add_subdirectory(public.sdk/samples/vst/validator)

#-------------------------------------------------------------------------------
# IDE sorting
#-------------------------------------------------------------------------------
set_target_properties(sdk PROPERTIES ${SDK_IDE_LIBS_FOLDER})
set_target_properties(adelay PROPERTIES COMPILE_FLAGS ${WARNINGS_DISABLED})
set_target_properties(sdk PROPERTIES COMPILE_FLAGS ${WARNINGS_DISABLED})
set_target_properties(base PROPERTIES COMPILE_FLAGS ${WARNINGS_DISABLED})
set_target_properties(validator PROPERTIES COMPILE_FLAGS ${WARNINGS_DISABLED})


if (TARGET adelay)
    set_target_properties(adelay PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
endif()
if (TARGET mda-vst3)
    set_target_properties(mda-vst3 PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
endif()
if (TARGET validator)
    set_target_properties(validator PROPERTIES ${SDK_IDE_HOSTING_EXAMPLES_FOLDER})
endif ()

if(MAC AND XCODE)
	if(SMTG_COREAUDIO_SDK_PATH)
		set_target_properties(auwrapper PROPERTIES ${SDK_IDE_LIBS_FOLDER})
		set_target_properties(again_au PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
	endif()
	if(IOS_DEVELOPMENT_TEAM)
		set_target_properties(sdk_ios PROPERTIES ${SDK_IDE_LIBS_FOLDER})
		set_target_properties(base_ios PROPERTIES ${SDK_IDE_LIBS_FOLDER})
		set_target_properties(interappaudio PROPERTIES ${SDK_IDE_LIBS_FOLDER})
		set_target_properties(noteexpressionsynth_ios PROPERTIES ${SDK_IDE_PLUGIN_EXAMPLES_FOLDER})
	endif()
endif()
