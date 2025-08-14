
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL
  "Suppress warnings from dependencies"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/utf8cpp/include)

# -------------------------------------------------------------------
# pygame
# -------------------------------------------------------------------
add_library(pygame STATIC "pygame/alphablit.cc")

# -------------------------------------------------------------------
# GLM
# -------------------------------------------------------------------
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glm)
target_include_directories(glm PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/glm
)

# -------------------------------------------------------------------
# libsamplerate
# -------------------------------------------------------------------
set(LIBSAMPLERATE_EXAMPLES OFF CACHE BOOL "Disable src examples" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable src tests" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate)
target_include_directories(samplerate PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate/include
)

# -------------------------------------------------------------------
# SDL_image
# -------------------------------------------------------------------
add_library(sdl_image STATIC SDL_image/src/IMG_xpm.c)
target_include_directories(sdl_image PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/include
)
target_include_directories(sdl_image PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/SDL_image/include/SDL
)
target_link_libraries(sdl_image PUBLIC ${SDL1_LIBRARIES})

# -------------------------------------------------------------------
# SDL_mixer
# -------------------------------------------------------------------
file(GLOB SDLMIXER_SRC_FILES "SDL_mixer/src/*.c")
add_library(sdl_mixer STATIC ${SDLMIXER_SRC_FILES})
target_include_directories(sdl_mixer PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/SDL_mixer/include
)
target_include_directories(sdl_mixer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/SDL_mixer/include/SDL
)
target_link_libraries(sdl_mixer PUBLIC ${SDL1_LIBRARIES})

# -------------------------------------------------------------------
# SDL_ttf
# -------------------------------------------------------------------
add_library(sdl_ttf STATIC SDL_ttf/src/SDL_ttf.c)
target_include_directories(sdl_ttf PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/SDL_ttf/include
)
target_link_libraries(sdl_ttf PUBLIC ${FREETYPE_LIBRARIES})
target_link_libraries(sdl_ttf PUBLIC ${SDL1_LIBRARIES})
target_compile_options(sdl_ttf PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-w>
)

# -------------------------------------------------------------------
# utf8cpp
# -------------------------------------------------------------------
add_library(utf8cpp INTERFACE)
target_include_directories(utf8cpp INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/utf8cpp/source
)

# -------------------------------------------------------------------
# xclannad
# -------------------------------------------------------------------
file(GLOB XCLANNAD_SRC_FILES "xclannad/*.cc" "xclannad/*.cpp")
add_library(xclannad STATIC ${XCLANNAD_SRC_FILES})
target_link_libraries(xclannad PUBLIC sdl_mixer)
target_link_libraries(xclannad PRIVATE JPEG::JPEG)
