
set(RLVM_REQUIRED_LIBRARIES
    ${SDL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${Boost_LIBRARIES}
    xclannad
    glew
    guichan
    pygame
    samplerate
    vorbisfile
    sdl_image
    sdl_ttf
    sdl_mixer
)

# -------------------------------------------------------------------
# Collect RLVM Source Files
# -------------------------------------------------------------------
file(GLOB_RECURSE RLVM_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/base/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/base/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/effects/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/effects/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/encodings/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/encodings/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/libreallive/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/libreallive/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/long_operations/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/long_operations/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/machine/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/machine/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/modules/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/object/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/object/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/utilities/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utilities/*.cc"

    "${CMAKE_CURRENT_SOURCE_DIR}/memory/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/memory/*.cc"
    
    "${CMAKE_CURRENT_SOURCE_DIR}/systems/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/systems/*.cc"
)
list(APPEND RLVM_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/platforms/platform_factory.cc"
)

if (GTK2_FOUND) # use gtk
   list(APPEND RLVM_SRC_FILES
       "${CMAKE_CURRENT_SOURCE_DIR}/platforms/gtk/gtk_implementor.cc"
   )
   list(APPEND RLVM_REQUIRED_LIBRARIES ${GTK2_LIBRARIES})
endif()

add_library(rlvm_src OBJECT ${RLVM_SRC_FILES})
target_include_directories(rlvm_src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(rlvm_src PRIVATE
  -Wno-narrowing
  -w
)

target_link_libraries(rlvm_src PUBLIC ${RLVM_REQUIRED_LIBRARIES})

# -------------------------------------------------------------------
# Build RLVM
# -------------------------------------------------------------------
set(SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/rlvm_instance.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/rlvm_main.cc"
)

add_executable(rlvm ${SRC_FILES})

target_link_libraries(rlvm PRIVATE
  rlvm_src
)
