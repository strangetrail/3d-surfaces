cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

project(3DSurfaces)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package( OpenGL REQUIRED )
find_package( glm CONFIG REQUIRED )
find_package( SDL2 CONFIG REQUIRED )

include_directories(SYSTEM anttweakbar/include)

add_subdirectory(anttweakbar EXCLUDE_FROM_ALL)

set(target 3dsurfaces.elf)

set(3dsurfaces_SOURCES
  src/main.cpp
  src/Shader.cpp
  src/SurfacePlotter.cpp
  src/GLProgram.cpp
  src/Camera.cpp
)

add_executable(${target} ${3dsurfaces_SOURCES})

target_link_libraries(${target}
  PRIVATE
  GL
  SDL2
  AntTweakBar
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

