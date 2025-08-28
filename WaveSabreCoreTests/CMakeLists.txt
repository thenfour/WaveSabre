# Unit test project for WaveSabreCore using GoogleTest
cmake_minimum_required(VERSION 3.14)

include(FetchContent)

# Fetch GoogleTest at a pinned, known-good commit/tag
# Note: GoogleTest uses C++14+; our project uses C++20 so it's fine.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # MSVC: use DLL runtime to align with default toolchain
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
# Prevent GoogleTest from overriding our compiler/linker options
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Collect all test sources recursively to avoid manual maintenance
file(GLOB_RECURSE TEST_SOURCES
  CONFIGURE_DEPENDS
  src/*.cpp
  src/*.cxx
  src/*.cc
)

# Group under "Tests" in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TEST_SOURCES})

# Define the test executable
add_executable(WaveSabreCoreTests ${TEST_SOURCES})

# Inherit the same C++ standard
set_target_properties(WaveSabreCoreTests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

# Link to GoogleTest and the library under test
# WaveSabreCore is defined at ../WaveSabreCore/CMakeLists.txt

target_link_libraries(WaveSabreCoreTests
  PRIVATE
    WaveSabreCore
    gtest
    gtest_main
)

# Ensure headers from WaveSabreCore are visible (already PUBLIC on the target, but safe)
# target_include_directories(WaveSabreCoreTests PRIVATE ${CMAKE_SOURCE_DIR}/WaveSabreCore/include)

# On MSVC, match typical debug info and exceptions off policy only for core code; tests can keep defaults.
if(MSVC)
  # Keep default runtime for tests, not the custom MSVCRT size-optimized flags.
  target_compile_options(WaveSabreCoreTests PRIVATE /W3 /EHsc)
endif()

# CTest integration: discover tests
include(GoogleTest)
gtest_discover_tests(WaveSabreCoreTests)
