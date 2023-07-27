set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.0)

project(go-make Go CXX)

include(cmake/golang.cmake)
include(cmake/flags.cmake)

add_go_library_with_modules(golib STATIC src/main.go)

add_executable(tst src/main.cpp)
target_link_libraries(tst golib)
