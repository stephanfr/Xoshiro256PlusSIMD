cmake_minimum_required(VERSION 3.14.0)
project(Xoshiro256Plus VERSION 0.9.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Determine if Xoshiro is built as a subproject (using add_subdirectory)
# or if it is the master project.
if (NOT DEFINED XOSHIRO256PLUS_IS_MASTER_PROJECT)
  set(XOSHIRO256PLUS_IS_MASTER_PROJECT OFF)
  if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(XOSHIRO256PLUS_IS_MASTER_PROJECT ON)
    message(STATUS "CMake version: ${CMAKE_VERSION}")
    message(STATUS "Xoshiro256Plus is master project")
  endif ()
endif ()

include_directories(
  )

if( XOSHIRO256PLUS_IS_MASTER_PROJECT )
  add_subdirectory(UnitTest)
  add_subdirectory(UnitTestNoAVX)
endif ()
