set(GOPATH "${CMAKE_CURRENT_BINARY_DIR}/go")
file(MAKE_DIRECTORY ${GOPATH})

function(GO_GET TARG)
  add_custom_target(${TARG} env GOPATH=${GOPATH} ${CMAKE_Go_COMPILER} get ${ARGN})
endfunction(GO_GET)

function(BUILD_GO_MODULE NAME SOURCE_FILE)
  cmake_path(GET SOURCE_FILE FILENAME SOURCE_FILE_NO_PATH)

  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/go.mod
    COMMAND env GOPATH=${GOPATH} ${CMAKE_Go_COMPILER} mod init ${SOURCE_FILE_NO_PATH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/go.sum
    COMMAND env GOPATH=${GOPATH} ${CMAKE_Go_COMPILER} mod tidy -e
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/go.mod)

  add_custom_target(${NAME} ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/go.mod ${CMAKE_CURRENT_SOURCE_DIR}/go.sum)
endfunction(BUILD_GO_MODULE)

function(ADD_GO_EXECUTABLE NAME GO_SOURCE)
  add_custom_command(OUTPUT ${OUTPUT_DIR}/.timestamp
    COMMAND env GOPATH=${GOPATH} ${CMAKE_Go_COMPILER} build
    -o "${CMAKE_CURRENT_BINARY_DIR}/${NAME}"
    ${CMAKE_GO_FLAGS} ${GO_SOURCE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

  add_custom_target(${NAME} ALL DEPENDS ${OUTPUT_DIR}/.timestamp ${ARGN})
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${NAME} DESTINATION bin)
endfunction(ADD_GO_EXECUTABLE)

function(ADD_GO_EXECUTABLE_WITH_MODULES NAME GO_SOURCE)
  build_go_module(${NAME}_module ${GO_SOURCE})
  ADD_GO_EXECUTABLE(${NAME} ${GO_SOURCE})
  add_dependencies(${NAME} ${NAME}_module)
endfunction(ADD_GO_EXECUTABLE_WITH_MODULES)


function(ADD_GO_LIBRARY NAME BUILD_TYPE GO_SOURCE)
  if(BUILD_TYPE STREQUAL "STATIC")
    set(BUILD_MODE -buildmode=c-archive)
    set(LIB_NAME "lib${NAME}.a")
  else()
    set(BUILD_MODE -buildmode=c-shared)
    if(APPLE)
      set(LIB_NAME "lib${NAME}.dylib")
    elseif(WIN32)
      set(LIB_NAME "lib${NAME}.dll")
    else()
      set(LIB_NAME "lib${NAME}.so")
    endif()
  endif()

  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}
    COMMAND env GOPATH=${GOPATH} ${CMAKE_Go_COMPILER} build ${BUILD_MODE}
      -o "${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}"
      ${CMAKE_GO_FLAGS} ${GO_SOURCE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    DEPENDS ${GO_SOURCE})

  add_custom_target(${NAME}_internal ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME})

  if(BUILD_TYPE STREQUAL "STATIC")
    add_library(${NAME} STATIC IMPORTED GLOBAL)
  else()
    add_library(${NAME} SHARED IMPORTED GLOBAL)
  endif()

  add_dependencies(${NAME} ${NAME}_internal)
  set_target_properties(${NAME}
    PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

  if(NOT BUILD_TYPE STREQUAL "STATIC")
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME} DESTINATION bin)
  endif()
endfunction(ADD_GO_LIBRARY)

function(ADD_GO_LIBRARY_WITH_MODULES NAME BUILD_TYPE GO_SOURCE)
  build_go_module(${NAME}_module ${GO_SOURCE})
  add_go_library(${NAME} ${BUILD_TYPE} ${GO_SOURCE})
  add_dependencies(${NAME} ${NAME}_module)
endfunction(ADD_GO_LIBRARY_WITH_MODULES)