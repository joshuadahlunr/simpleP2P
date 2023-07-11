set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.12)

project(go-make Go CXX)

include(cmake/golang.cmake)
include(cmake/flags.cmake)

add_go_library_with_modules(golib STATIC src/main.go)

add_subdirectory(extern/argparse)

add_executable(tst src/main.cpp)
target_link_libraries(tst golib argparse)
set_property(TARGET tst PROPERTY CXX_STANDARD 20)
