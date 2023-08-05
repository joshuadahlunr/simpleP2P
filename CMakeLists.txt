set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.20)

if(CMAKE_VERSION VERSION_LESS 3.21)
	get_property(not_top DIRECTORY PROPERTY PARENT_DIRECTORY)
	if(not_top)
		set(PROJECT_IS_TOP_LEVEL off)
	else()
		set(PROJECT_IS_TOP_LEVEL true)
	endif()
endif()

option(BUILD_EXAMPLES "Build the example applications" ${PROJECT_IS_TOP_LEVEL})

project(simplep2p Go CXX C)

include(cmake/golang.cmake)
include(cmake/flags.cmake)

# If we are compiling on windows overwrite the GOROOT based on the compiler path
if(WIN32)
	cmake_path(REMOVE_FILENAME CMAKE_Go_COMPILER OUTPUT_VARIABLE GOROOT)
	cmake_path(APPEND GOROOT "../lib/go")
	cmake_path(ABSOLUTE_PATH GOROOT NORMALIZE)
	message("Overwriting GOROOT with ${GOROOT}")
endif()

add_go_library_with_modules(simplep2p_golib STATIC src/libp2p.go)
target_go_get_dependency(simplep2p_golib NAME get_libp2p PACKAGES github.com/libp2p/go-libp2p)

add_library(simplep2p STATIC src/simplep2p.c)
target_include_directories(simplep2p PUBLIC src)
target_link_libraries(simplep2p simplep2p_golib)

if(BUILD_EXAMPLES)
	add_subdirectory(examples/argparse)

	add_executable(chat examples/chat.cpp)
	target_link_libraries(chat simplep2p argparse)
	set_property(TARGET chat PROPERTY CXX_STANDARD 20)

	add_executable(chat.capi examples/chat.capi.cpp)
	target_link_libraries(chat.capi simplep2p argparse)
	set_property(TARGET chat.capi PROPERTY CXX_STANDARD 20)
	if(MSVC)
		target_link_libraries(chat.capi legacy_stdio_definitions.lib)
	endif()
endif()
