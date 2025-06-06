
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h"
  @ONLY
)

set(RLVM_REQUIRED_LIBRARIES
    ${SDL1_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${GLEW_LIBRARIES}
    VorbisFile::VorbisFile
    glm::glm
    xclannad
    pygame
    samplerate
    sdl_image
    sdl_ttf
    sdl_mixer
    utf8cpp
    TBB::tbb
)

# -------------------------------------------------------------------
# Collect RLVM Source Files
# -------------------------------------------------------------------
file(GLOB_RECURSE RLVM_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/effects/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/encodings/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/libreallive/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/libsiglus/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/long_operations/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/machine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/object/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utilities/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/systems/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/log/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/m6/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/vm/*.cpp"
)
list(APPEND RLVM_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/platforms/platform_factory.cpp"
)

if (GTK2_FOUND) # use gtk
   list(APPEND RLVM_SRC_FILES
       "${CMAKE_CURRENT_SOURCE_DIR}/platforms/gtk/gtk_implementor.cpp"
   )
   list(APPEND RLVM_REQUIRED_LIBRARIES ${GTK2_LIBRARIES})
endif()

add_library(rlvm_src OBJECT)
target_sources(rlvm_src PRIVATE ${RLVM_SRC_FILES})
target_include_directories(rlvm_src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(rlvm_src PUBLIC
  $<$<CXX_COMPILER_ID:GNU,Clang>:-funsigned-char -Wall -Wno-narrowing -Wno-write-strings
  -Wno-sign-compare>
)

target_link_libraries(rlvm_src PUBLIC ${RLVM_REQUIRED_LIBRARIES})
target_include_directories(rlvm_src PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")

# -------------------------------------------------------------------
# Build RLVM
# -------------------------------------------------------------------
set(RLVM_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/rlvm_instance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/rlvm_main.cpp"
)

add_executable(rlvm ${RLVM_FILES})

target_link_libraries(rlvm PRIVATE
  rlvm_src
)

# -------------------------------------------------------------------
# Build RLKP
# -------------------------------------------------------------------
set(RLKP_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/rlkp_main.cpp"
)

add_executable(rlkp ${RLKP_FILES})

target_link_libraries(rlkp PRIVATE rlvm_src)

set(SL_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/repl.cpp"
)
add_executable(sl ${SL_FILES})
target_link_libraries(sl PRIVATE rlvm_src)
