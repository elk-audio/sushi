
######################
#  Executable target #
######################

set(APP_FILES main.cpp)

if(APPLE)
    add_executable(sushi MACOSX_BUNDLE ${APP_FILES})
else()
    add_executable(sushi ${APP_FILES})
endif()

####################################
#  Compiler Flags and definitions  #
####################################

if(MSVC)
    # C4996: Deprecated warning. This was flooding the terminal,
    #        I might remove the suppression from here eventually,
    #        and out into the code again.
    set(SUSHI_COMPILE_OPTIONS /W4 /fp:fast /wd4996)
    set(APP_LINK_LIBRARIES ${APP_LINK_LIBRARIES} winmm.lib) # oscpack GetCurrentTimeMs
else()
    set(SUSHI_COMPILE_OPTIONS -Wall -Wextra -Wno-psabi -fPIC -fno-rtti -ffast-math)
endif()

target_compile_options(sushi PRIVATE ${SUSHI_COMPILE_OPTIONS})

target_link_libraries(sushi PRIVATE sushi_library ${APP_LINK_LIBRARIES})

####################
#  Install         #
####################

set(DOC_FILES_INSTALL
        "${PROJECT_SOURCE_DIR}/LICENSE.md"
        "${PROJECT_SOURCE_DIR}/README.md"
        "${PROJECT_SOURCE_DIR}/HISTORY.md"
        )

if (APPLE)
    # TODO: fix to work when not submodule
    install(CODE [[
    include(BundleUtilities)
    fixup_bundle("${CMAKE_INSTALL_PREFIX}/sushi/sushi.app" "" "")
    ]] COMPONENT Runtime)
endif()

### Custom command to copy the dynamic dependencies to the binary folder
#   Mainly for twine.dll because windows cannot find it from ../twine but
#   needs it in the same directory
if (MSVC)
    add_custom_command(TARGET sushi POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:sushi> $<TARGET_FILE_DIR:sushi>
            COMMAND_EXPAND_LISTS)
endif()

install(TARGETS sushi RUNTIME DESTINATION bin BUNDLE DESTINATION Applications)
foreach(ITEM ${DOC_FILES_INSTALL})
    install(FILES ${ITEM} DESTINATION share/sushi/doc)
endforeach()
