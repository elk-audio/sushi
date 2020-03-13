####################
#  CMake packages  #
####################

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/rpc_interface/cmake)

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)

################################
#  gRPC / Protobuf generation  #
################################

set(PROTOS protos/sushi_rpc.proto)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

# Supress warnings in auto-generated files
foreach(ITEM ${PROTO_SRCS} ${GRPC_SRCS})
    set_source_files_properties(${ITEM} PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
endforeach()

######################
#  Library target    #
######################

set(SUSHI_GRPC_SOURCES src/grpc_server.cpp
                       src/control_service.cpp )

add_library(sushi_rpc STATIC
                      ${SUSHI_GRPC_SOURCES}
                      ${PROTO_SRCS}
                      ${GRPC_SRCS} )

target_include_directories(sushi_rpc PUBLIC include)
target_link_libraries(sushi_rpc gRPC::grpc++_reflection protobuf::libprotobuf gpr)
target_compile_features(sushi_rpc PRIVATE cxx_std_17)
target_compile_options(sushi_rpc PRIVATE -Wall -Wextra )

target_include_directories(sushi_rpc PRIVATE ${PROTO_SRC_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
target_include_directories(sushi_rpc PUBLIC include)

##################
#  Install step  #
##################

install(FILES ${PROTOS} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/sushi)
