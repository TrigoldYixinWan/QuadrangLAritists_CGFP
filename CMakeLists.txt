cmake_minimum_required(VERSION 3.16)

# Sets project name
project(projects_realtime LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Sets C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specifies required Qt components
find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL OpenGLWidgets Xml)

# Find Box2D installed via vcpkg


# Allows you to include files from within those directories, without prefixing their filepaths
include_directories(src)

# Specifies .cpp and .h files to be passed to the compiler
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/realtime.cpp
    src/mainwindow.cpp
    src/settings.cpp
    src/utils/scenefilereader.cpp
    src/utils/sceneparser.cpp
    src/mainwindow.h
    src/realtime.h
    src/settings.h
    src/utils/scenedata.h
    src/utils/scenefilereader.h
    src/utils/sceneparser.h
    src/utils/shaderloader.h
    src/utils/aspectratiowidget/aspectratiowidget.hpp
    src/utils/cone.h src/utils/cone.cpp
    src/utils/cube.h src/utils/cube.cpp
    src/utils/cylinder.h src/utils/cylinder.cpp
    src/utils/sphere.h src/utils/sphere.cpp
    src/camera.h src/camera.cpp
)

# GLM: this creates its library and allows you to #include "glm/..."
add_subdirectory(glm)
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# Update your target_link_libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::Xml
    StaticGLEW
    ${CMAKE_SOURCE_DIR}/lib/libliquidfun.a
)
# GLEW: this creates its library and allows you to #include "GL/glew.h"
add_library(StaticGLEW STATIC glew/src/glew.c
    src/utils/cone.h src/utils/cone.cpp)
include_directories(${PROJECT_NAME} PRIVATE glew/include)

# Specifies libraries to be linked (Qt components, glew, Box2D, etc)


# Specifies other files
qt6_add_resources(${PROJECT_NAME} "Resources"
    PREFIX "/"
    FILES
        resources/shaders/default.frag
        resources/shaders/default.vert
        resources/shaders/screen_quad.vert
        resources/shaders/post_processing.frag
        resources/shaders/2D.frag
        resources/shaders/2D.vert
        resources/planetText/test.png
        resources/planetText/earth.png
        resources/planetText/jupiter.png
        resources/planetText/mars.png
        resources/planetText/neptune.png
        resources/planetText/saturn.png
        resources/planetText/uranus.png
        resources/planetText/venus.png
)

# GLEW: this provides support for Windows (including 64-bit)
if (WIN32)
  add_compile_definitions(GLEW_STATIC)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32
    glu32
  )
endif()

# Set this flag to silence warnings on Windows
if (MSVC OR MSYS OR MINGW)
  set(CMAKE_CXX_FLAGS "-Wno-volatile")
endif()

# Set this flag to silence warnings on MacOS
if (APPLE)
  set(CMAKE_CXX_FLAGS "-Wno-deprecated-volatile")
endif()

