cmake_minimum_required(VERSION 3.18)
project(ArgParser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB ARG_PARSER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_library(ArgParser STATIC ${ARG_PARSER_SOURCE})

target_include_directories(ArgParser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)