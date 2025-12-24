cmake_minimum_required(VERSION 3.18)
project(Hasher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB HASHER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_library(Hasher STATIC ${HASHER_SOURCE})

target_include_directories(Hasher PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)