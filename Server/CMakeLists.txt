cmake_minimum_required(VERSION 3.18)
project(Server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

file(GLOB SERVER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(Server ${SERVER_SOURCES})

target_include_directories(Server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Server PRIVATE
	FTPService
        FileStream
        gRPC::grpc++
        spdlog
        fmt
        ${Protobuf_LIBRARIES}
)
