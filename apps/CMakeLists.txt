
######################
#  Executable target #
######################

set(APP_FILES main.cpp)

add_executable(sushi MACOSX_BUNDLE ${APP_FILES})

####################################
#  Compiler Flags and definitions  #
####################################

if(MSVC)    
    set(SUSHI_COMPILE_OPTIONS /W4 /fp:fast)
else()
    set(SUSHI_COMPILE_OPTIONS -Wall -Wextra -Wno-psabi -fPIC -fno-rtti -ffast-math)
endif()

target_compile_options(sushi PRIVATE ${SUSHI_COMPILE_OPTIONS})

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
    # TODO: fix to work when not submodule
    install(CODE [[
    include(BundleUtilities)
    fixup_bundle("${CMAKE_INSTALL_PREFIX}/sushi/sushi.app" "" "")
    ]] COMPONENT Runtime)
endif()

install(TARGETS sushi RUNTIME DESTINATION bin, BUNDLE DESTINATION Applications)
foreach(ITEM ${DOC_FILES_INSTALL})
    install(FILES ${ITEM} DESTINATION share/sushi/doc)
endforeach()