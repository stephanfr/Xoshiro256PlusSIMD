
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.0-preview3)

FetchContent_MakeAvailable(Catch2)

# Assume the platform supports AVX2 - if not, then this project is not terribly useful.

SET( AVX_FLAGS "-mavx2 -D__AVX2_AVAILABLE__" )

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${AVX_FLAGS}")

include_directories()

link_directories()

add_executable( tests
  BasicTests.cpp
  Benchmark.cpp
)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

catch_discover_tests( tests )
