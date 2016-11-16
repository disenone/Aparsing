cmake_minimum_required(VERSION 2.6.2)
project(parsingargs CXX C)
set(CMAKE_VERBOSE_MAKEFILE ON)

include(internal_utils.cmake)
config_compiler_and_linker()

message("cxx flags ${CMAKE_CXX_FLAGS}")
message("c flags ${CMAKE_C_FLAGS}")

# getopt
set(GETOPT_DIR ${PROJECT_SOURCE_DIR}/getopt)
aux_source_directory(${GETOPT_DIR} GETOPT_SOURCES) 
list(REMOVE_ITEM GETOPT_SOURCES "${GETOPT_DIR}/getopt_test.c")
add_library(getopt SHARED ${GETOPT_SOURCES})
include_directories(${GETOPT_DIR})
add_executable(getopt_test "${GETOPT_DIR}/getopt_test.c")
target_link_libraries(getopt_test getopt) 


# getenv
set(GETENV_DIR ${PROJECT_SOURCE_DIR}/getenv)
add_executable(getenv_test "${GETENV_DIR}/getenv_test.c")


# getparams
set(MODULEPARAM_DIR ${PROJECT_SOURCE_DIR}/moduleparam)
add_library(moduleparam SHARED ${MODULEPARAM_DIR}/moduleparam.c)
add_executable(moduleparam_test "${MODULEPARAM_DIR}/moduleparam_test.c")
target_link_libraries(moduleparam_test moduleparam) 

