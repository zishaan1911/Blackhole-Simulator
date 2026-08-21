cmake_minimum_required(VERSION 3.16)
project(KerrBlackHole LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

# ---------------------------------------------------------------------------
# GLFW - use the system package if there is one, otherwise fetch it.
# ---------------------------------------------------------------------------
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW not found on the system - fetching 3.4")
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(glfw)
endif()

# ---------------------------------------------------------------------------
# GLM (header only)
# ---------------------------------------------------------------------------
find_package(glm QUIET)
if(NOT glm_FOUND)
    message(STATUS "GLM not found on the system - fetching 1.0.1")
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(glm)
endif()

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
add_executable(kerr
    src/main.cpp
    src/BlackHole.cpp
    src/Camera.cpp
    src/Renderer.cpp
    src/Simulation.cpp
    src/Shader.cpp
    src/glad_min.cpp
    src/PngWriter.cpp
)

target_include_directories(kerr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Absolute fallback path so the binary finds its shaders no matter where it is
# launched from.
target_compile_definitions(kerr PRIVATE
    SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

if(TARGET glm::glm)
    target_link_libraries(kerr PRIVATE glfw glm::glm)
else()
    target_link_libraries(kerr PRIVATE glfw glm)
endif()

