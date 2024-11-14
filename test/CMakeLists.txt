
cmake_minimum_required(VERSION 3.14)
project(RLVMTests)

find_package(GTest QUIET)

if (GTest_FOUND)
    message(STATUS "Found GoogleTest: ${GTest_INCLUDE_DIRS}")
else()
    message(STATUS "GoogleTest not found. Fetching using FetchContent.")
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        # Optional: Specify a particular version or tag
        # URL https://github.com/google/googletest/archive/release-1.11.0.zip
    )

    # Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()
file(GLOB_RECURSE TEST_SRC_FILES
    "*.cpp"
    "*.cc"
)
add_executable(unittest ${TEST_SRC_FILES})
target_compile_options(unittest PRIVATE
  -Wno-narrowing
  -Wall
)

if (GTest_FOUND)
    target_link_libraries(unittest
        GTest::gtest_main
        GTest::gmock_main
    )
else()
    target_link_libraries(unittest
        gtest_main
        gmock_main
    )
endif()

include(GoogleTest)
gtest_discover_tests(unittest)
