cmake_minimum_required(VERSION 3.5.1)
project(L2S)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#ADD_DEFINITIONS(-DDEBUG)

include_directories(./parser ./train ./basic ./solver . ./blaze ./matrix)


file(GLOB basic_files
        "basic/*.h"
        "basic/*.cpp"
        "matrix/*.h"
        "matrix/*.cpp"
        )

file(GLOB parser_files
        "parser/*.h"
        "parser/*.cpp")

file(GLOB solver_files
        "solver/*.h"
        "solver/*.cpp"
        )

add_library(basic_lib ${basic_files})
add_library(parser_lib ${parser_files})
add_library(solver_lib ${solver_files})

INCLUDE(FindPkgConfig)
find_package(Jsoncpp)
include_directories(${Jsoncpp_INCLUDE_DIR})

add_executable(run main/run.cpp)
target_link_libraries(run basic_lib parser_lib solver_lib basic_lib ${Jsoncpp_LIBRARY} gflags glog)