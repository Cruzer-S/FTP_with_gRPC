cmake_minimum_required(VERSION 3.18)
project(Client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

file(GLOB CLIENT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(Client ${CLIENT_SOURCES})

target_include_directories(Client PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Client PRIVATE
        protocol
        gRPC::grpc++
        spdlog
        fmt
        stdc++fs
        Hasher
        FileStream
        ${Protobuf_LIBRARIES}
)