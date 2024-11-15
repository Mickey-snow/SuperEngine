cmake_minimum_required(VERSION 3.15)

project(RLVM VERSION 0.14.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
option(RLVM_BUILD_TESTS "Build unit test program" OFF)
option(RLVM_FULLSTATIC "Build executables that are linked statically" OFF)

if(RLVM_FULLSTATIC)
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# -------------------------------------------------------------------
# Find OpenGL Library
# -------------------------------------------------------------------
find_package(OpenGL REQUIRED)
if (OpenGL_FOUND)
  message(STATUS "OpenGL found: ${OpenGL_VERSION}")
  include_directories(${OpenGL_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "OpenGL not found. Please install OpenGL.")
endif()

# -------------------------------------------------------------------
# Find SDL1 Library
# -------------------------------------------------------------------
find_package(SDL REQUIRED)

if (SDL_FOUND)
  message(STATUS "SDL1 found: ${SDL_VERSION}")
  include_directories(${SDL_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "SDL1 not found. Please install SDL1.2 development libraries.")
endif()

# -------------------------------------------------------------------
# Find Boost Libraries
# -------------------------------------------------------------------
set(BOOST_COMPONENTS program_options serialization iostreams filesystem)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

if (Boost_FOUND)
  message(STATUS "Boost found: ${Boost_VERSION}")
  include_directories(${Boost_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Boost libraries not found. Please install Boost.")
endif()

# -------------------------------------------------------------------
# Find Freetype2 Library
# -------------------------------------------------------------------
find_package(Freetype REQUIRED)

if (Freetype_FOUND)
  message(STATUS "Freetype2 found: ${Freetype_VERSION}")
  include_directories(${FREETYPE_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Freetype2 not found. Please install Freetype2 development libraries.")
endif()


# -------------------------------------------------------------------
# Find zlib Library
# -------------------------------------------------------------------
find_package(ZLIB REQUIRED)

if (ZLIB_FOUND)
  message(STATUS "zlib found: ${ZLIB_VERSION}")
  include_directories(${ZLIB_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "zlib not found. Please install zlib development libraries.")
endif()

# -------------------------------------------------------------------
# Find Ogg Library
# -------------------------------------------------------------------
find_package(Ogg REQUIRED)

if (Ogg_FOUND)
  message(STATUS "Ogg found: ${OGG_VERSION}")
  include_directories(${OGG_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "Ogg not found. Please install libogg development libraries.")
endif()

# -------------------------------------------------------------------
# Find GTK 2.0 Library (optional)
# -------------------------------------------------------------------
find_package(GTK2 QUIET)

if (GTK2_FOUND)
  message(STATUS "GTK 2.0 found.")
  include_directories(${GTK2_INCLUDE_DIRS})
else()
  message(STATUS "GTK 2.0 not found. Continuing without GTK 2.0 support.")
endif()

# -------------------------------------------------------------------
# Add Subdirectories to Build
# -------------------------------------------------------------------
include_directories(vendor)
add_subdirectory(vendor)
add_subdirectory(src)
if(RLVM_BUILD_TESTS)
  add_subdirectory(test)
endif()
