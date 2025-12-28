cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(GEN_SRCS "")
set(GEN_HDRS "")
set(LIB_NAME "FTPService")

file(GLOB PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)
file(GLOB PROTOCOL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

file(MAKE_DIRECTORY ${GEN_DIR})

foreach(PROTO ${PROTO_FILES})
        get_filename_component(PROTO_NAME ${PROTO} NAME_WE)
        list(APPEND GEN_SRCS
                ${GEN_DIR}/${PROTO_NAME}.pb.cc
                ${GEN_DIR}/${PROTO_NAME}.grpc.pb.cc
        )
        list(APPEND GEN_HDRS
                ${GEN_DIR}/${PROTO_NAME}.pb.h
                ${GEN_DIR}/${PROTO_NAME}.grpc.pb.h
        )

        add_custom_command(
                OUTPUT
                        "${GEN_DIR}/${PROTO_NAME}.pb.cc"
                        "${GEN_DIR}/${PROTO_NAME}.pb.h"
                        "${GEN_DIR}/${PROTO_NAME}.grpc.pb.cc"
                        "${GEN_DIR}/${PROTO_NAME}.grpc.pb.h"
                COMMAND ${Protobuf_PROTOC_EXECUTABLE}
                        -I ${CMAKE_CURRENT_SOURCE_DIR}
                        --cpp_out=${GEN_DIR}
                        --grpc_out=${GEN_DIR}
                        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                        ${PROTO}
                DEPENDS ${PROTO}
        )
endforeach()

add_library(${LIB_NAME} STATIC ${GEN_SRCS} ${GEN_HDRS} ${PROTOCOL_SOURCES})

target_include_directories(${LIB_NAME}
        PUBLIC ${GEN_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${LIB_NAME} PUBLIC
        gRPC::grpc++
        ${Protobuf_LIBRARIES}
)
