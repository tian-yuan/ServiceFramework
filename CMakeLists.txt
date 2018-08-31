cmake_minimum_required(VERSION 2.8.12)

project(ServiceFramework C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CXX_FLAGS
 -g
 # -DVALGRIND
 # -DCHECK_PTHREAD_RETURN_VALUE
 #-D_FILE_OFFSET_BITS=64
#-Wall
 -w
 -Wextra
 # -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wno-sign-conversion
 -Wno-old-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 # -Wno-deprecated
 -DNEW_HEADER=1
 
 -march=native
 # -MMD
 -std=gnu++1y

 # -Wold-style-cast
 # -Wunused-parameter
 )

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")


if(APPLE)
  #set(CMAKE_CXX_COMPILER "g++-4.9")
  #set(CMAKE_CXX_COMPILER "clang++")
  message("APPLE")
  set(CMAKE_CXX_COMPILER "g++")
elseif(UNIX)
  set(CMAKE_CXX_COMPILER "g++")
  message("UNIX")
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/baselib/)

### for other libs
link_directories(/usr/local/ssl/lib /usr/local/lib)

#string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

if(APPLE)
elseif(UNIX)
set(CMAKE_EXE_LINKER_FLAGS "-lpthread -lrt -ldl")
endif()

add_subdirectory(baselib)
add_subdirectory(server)
