####################
#  CMake packages  #
####################

find_package(gRPC REQUIRED)

################################
#  gRPC / Protobuf generation  #
################################

set(PROTOS protos/sushi_rpc.proto)

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
    "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
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

