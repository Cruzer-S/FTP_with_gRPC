cmake_minimum_required(VERSION 3.18)
project(FileStream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB FILE_STREAM_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_library(FileStream STATIC ${FILE_STREAM_SOURCE})

target_link_libraries(FileStream
    PUBLIC Hasher
    PRIVATE fmt
)

target_include_directories(FileStream PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)