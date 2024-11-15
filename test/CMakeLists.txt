
project(RLVMTests)

# -------------------------------------------------------------------
# Find Googletest Library
# -------------------------------------------------------------------
find_package(GTest QUIET)

if (GTest_FOUND)
    message(STATUS "Found GoogleTest: ${GTest_INCLUDE_DIRS}")
    set(GTEST_LIBRARIES GTest::gtest_main GTest::gmock_main)
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
    set(GTEST_LIBRARIES gtest_main gmock_main)
endif()

enable_testing()

# -------------------------------------------------------------------
# Build Unittest
# -------------------------------------------------------------------
file(GLOB_RECURSE TEST_SRC_FILES
    "*.cpp"
    "*.cc"
)
add_executable(unittest ${TEST_SRC_FILES})
target_compile_options(unittest PRIVATE
  -Wno-narrowing
  -Wall
  -funsigned-char
)

target_link_libraries(unittest ${GTEST_LIBRARIES} rlvm_src)

# -------------------------------------------------------------------
# Prepare test files
# -------------------------------------------------------------------
add_custom_target(copy_gameexe ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Gameexe_data ${CMAKE_BINARY_DIR}/test/Gameexe_data
    DEPENDS ${DIRS_TO_COPY}
)
add_custom_target(copy_gameroot ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Gameroot ${CMAKE_BINARY_DIR}/test/Gameroot
    DEPENDS ${DIRS_TO_COPY}
)
add_dependencies(unittest copy_gameroot copy_gameexe)

find_package(PythonInterp REQUIRED)

# Generate wav test files
set(SAMPLE_RATES 8000 16000 44100)
set(CHANNELS 1 2)
set(SAMPLE_WIDTHS 8 16 32)
set(WAV_FILES)
set(WAV_PARAMS)
set(WAV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/test/Gameroot/WAV")
file(MAKE_DIRECTORY "${WAV_OUTPUT_DIR}")
set(SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/wav_generator.py")

set(INDEX 0)
foreach(SAMPLE_RATE ${SAMPLE_RATES})
  foreach(CHANNEL ${CHANNELS})
    foreach(SAMPLE_WIDTH ${SAMPLE_WIDTHS})
      set(FREQUENCY 440)
      set(DURATION 0.5)
      set(FILE_NAME "${WAV_OUTPUT_DIR}/test${INDEX}.wav")
      set(PARAM "--frequency=${FREQUENCY} --sample_rate=${SAMPLE_RATE} --channel=${CHANNEL} --sample_format=${SAMPLE_WIDTH} --duration=${DURATION}")

      add_custom_command(
          OUTPUT ${FILE_NAME}
	  COMMAND ${PYTHON_EXECUTABLE} ${SCRIPT_FILE} ${FILE_NAME} --frequency=${FREQUENCY} --sample_rate=${SAMPLE_RATE} --channel=${CHANNEL} --sample_format=${SAMPLE_WIDTH} --duration=${DURATION}
	  DEPENDS ${SCRIPT_FILE}
	  COMMENT "Generating ${FILE_NAME}"
      )

      list(APPEND WAV_FILES "${FILE_NAME}")

      math(EXPR INDEX "${INDEX} + 1")
    endforeach()
  endforeach()
endforeach()

add_custom_target(generate_wav_files ALL DEPENDS ${WAV_FILES})
add_dependencies(unittest generate_wav_files)

# Generate ogg test files
set(OGG_TARGET "${CMAKE_BINARY_DIR}/test/Gameroot/OGG/test.ogg")
set(SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/ogg_generator.py")
add_custom_command(
    OUTPUT ${OGG_TARGET}
    COMMAND ${PYTHON_EXECUTABLE} ${SCRIPT_FILE} ${OGG_TARGET}
    DEPENDS ${SCRIPT_FILE}
    COMMENT "Generating ${OGG_TARGET}"
)
add_custom_target(generate_ogg_files ALL DEPENDS ${OGG_TARGET})
add_dependencies(unittest generate_ogg_files)
