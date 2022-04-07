
######################
#  Executable target #
######################

add_executable(sushi MACOSX_BUNDLE main.cpp)

####################################
#  Compiler Flags and definitions  #
####################################

target_compile_features(sushi PRIVATE cxx_std_17)
target_compile_options(sushi PRIVATE -Wall -fPIC -Wextra -Wno-psabi -fno-rtti -ffast-math -fno-rtti)

target_compile_options(sushi_library PRIVATE -Wall -fPIC -Wextra -Wno-psabi -fno-rtti -ffast-math)
target_link_libraries(sushi PRIVATE sushi_library)

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