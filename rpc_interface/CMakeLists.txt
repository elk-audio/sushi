####################
#  CMake packages  #
####################

find_package(gRPC REQUIRED)


################################
#  gRPC / Protobuf generation  #
################################

set(PROTOS protos/sushi_rpc.proto)

# In yocto environments, CMAKE_CROSSCOMPILING is set. This if condition is needed
# as yocto grpc does not include gRPCPluginTargets.cmake when find_package(gRPC REQUIRED)
# is called. 
if(CMAKE_CROSSCOMPILING)
    find_program(_gRPC_CPP_PLUGIN grpc_cpp_plugin)
else()
    set(_gRPC_CPP_PLUGIN $<TARGET_FILE:grpc_cpp_plugin>)
endif()

# Intermediate library target with the generated protobuf objects
add_library(protos-lib OBJECT)
target_sources(protos-lib PRIVATE ${PROTOS})
target_link_libraries(protos-lib PUBLIC gRPC::grpc++)
target_include_directories(protos-lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate(TARGET protos-lib LANGUAGE cpp)
protobuf_generate(
    TARGET protos-lib
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN
    "protoc-gen-grpc=${_gRPC_CPP_PLUGIN}"
)

######################
#  Library target    #
######################

set(SUSHI_GRPC_SOURCES src/grpc_server.cpp
                       src/control_service.cpp
                       src/async_service_call_data.cpp )

set(SUSHI_RPC_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/protos
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${PROJECT_SOURCE_DIR}/include
                           ${PROJECT_SOURCE_DIR}/src )

add_library(sushi_rpc STATIC ${SUSHI_GRPC_SOURCES})

target_include_directories(sushi_rpc PUBLIC include)
target_include_directories(sushi_rpc PRIVATE ${SUSHI_RPC_INCLUDE_DIRS})
target_link_libraries(sushi_rpc PUBLIC gRPC::grpc++ protos-lib)
target_compile_features(sushi_rpc PRIVATE cxx_std_17)
target_compile_options(sushi_rpc PRIVATE -Wall -Wextra )

##################
#  Install step  #
##################

install(FILES ${PROTOS} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/sushi)
