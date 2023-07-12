set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.12)

project(go-make Go CXX C)

include(cmake/golang.cmake)
include(cmake/flags.cmake)

add_go_library_with_modules(golib STATIC src/libp2p.go)

add_library(simplep2p STATIC src/simplep2p.c)
target_include_directories(simplep2p PUBLIC src)
target_link_libraries(simplep2p golib)

add_subdirectory(extern/argparse)

add_executable(tst src/main.cpp)
target_link_libraries(tst simplep2p argparse)
set_property(TARGET tst PROPERTY CXX_STANDARD 20)
