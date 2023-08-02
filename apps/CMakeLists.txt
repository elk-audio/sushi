
######################
#  Executable target #
######################

set(APP_FILES main.cpp)

if (${SUSHI_WITH_APPLE_COREAUDIO})
    set(APP_FILES ${APP_FILES}
        ../src/audio_frontends/apple_coreaudio/apple_coreaudio_object.cpp
        ../src/audio_frontends/apple_coreaudio/apple_coreaudio_device.mm
        ../src/audio_frontends/apple_coreaudio/apple_coreaudio_utils.cpp)
endif()

set(APP_FILES ${APP_FILES} ${FREEVERB_SOURCES})

add_executable(sushi MACOSX_BUNDLE ${APP_FILES})

####################################
#  Compiler Flags and definitions  #
####################################

target_compile_features(sushi PRIVATE cxx_std_17)
target_compile_options(sushi PRIVATE -Wall -Wextra -Wno-psabi -fPIC -fno-rtti -ffast-math)

target_link_libraries(sushi PRIVATE sushi_library "${APP_LINK_LIBRARIES}")

####################
#  Install         #
####################

set(DOC_FILES_INSTALL
        "${PROJECT_SOURCE_DIR}/LICENSE.md"
        "${PROJECT_SOURCE_DIR}/README.md"
        "${PROJECT_SOURCE_DIR}/HISTORY.md"
        )

if (APPLE)
    install(CODE [[
    include(BundleUtilities)
    fixup_bundle("${CMAKE_INSTALL_PREFIX}/sushi/sushi.app" "" "") # TODO: fix to work when not submodule
    ]] COMPONENT Runtime)
endif()

install(TARGETS sushi RUNTIME DESTINATION bin, BUNDLE DESTINATION Applications)
foreach(ITEM ${DOC_FILES_INSTALL})
    install(FILES ${ITEM} DESTINATION share/sushi/doc)
endforeach()