cmake_minimum_required(VERSION 3.16)
project(Kerrscope VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Build identity. Every binary can say what it is, so a bug report can name a
# build rather than "the latest one I think".
#
# The commit is resolved at configure time, so it goes stale if you keep
# building after new commits without re-running cmake. That is a fair trade for
# not shelling out to git on every compile; CI passes KERRSCOPE_COMMIT
# explicitly, and a build from a source tarball with no .git reports "unknown".
# ---------------------------------------------------------------------------
if(NOT DEFINED KERRSCOPE_COMMIT)
    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE KERRSCOPE_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    endif()
endif()
if(NOT KERRSCOPE_COMMIT)
    set(KERRSCOPE_COMMIT "unknown")
endif()

string(TIMESTAMP KERRSCOPE_DATE "%Y-%m-%d" UTC)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.hpp"
    @ONLY)

option(KERRSCOPE_STATIC_RUNTIME
       "Link the C++ runtime statically, for a binary that runs without a redistributable"
       OFF)

# This has to be set before the dependencies are declared, not on the kerr
# target afterwards. GLFW is built from source in this same tree, and MSVC
# refuses to link objects that disagree about the runtime library (LNK2038,
# MT_StaticRelease against MD_DynamicRelease). Setting the variable here puts
# every target in the build - ours and GLFW's - on the same runtime.
if(KERRSCOPE_STATIC_RUNTIME AND MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # GLFW 3.3 predates CMAKE_MSVC_RUNTIME_LIBRARY and uses its own switch.
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
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
    src/Bodies.cpp
)

target_include_directories(kerr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Absolute fallback path so the binary finds its shaders no matter where it is
# launched from.
target_compile_definitions(kerr PRIVATE
    SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

if(TARGET glm::glm)
    target_link_libraries(kerr PRIVATE glfw glm::glm)
else()
    target_link_libraries(kerr PRIVATE glfw glm)
endif()

if(MSVC)
    target_compile_options(kerr PRIVATE /W3)
else()
    target_compile_options(kerr PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

if(MINGW)
    target_link_options(kerr PRIVATE -static-libgcc -static-libstdc++)
endif()

# Release binaries should run on a machine that has never had a compiler on
# it. MinGW is already static above; this covers MSVC and static libstdc++ on
# GCC/Clang. Off by default, because a development build has no use for it.
# MSVC is handled up top, before the dependencies are declared. This is the
# GCC/Clang half, which only concerns our own target.
if(KERRSCOPE_STATIC_RUNTIME AND NOT MSVC AND NOT MINGW AND NOT APPLE)
    target_link_options(kerr PRIVATE -static-libgcc -static-libstdc++)
endif()

# Copy the shaders next to the executable for a self-contained build dir.
add_custom_command(TARGET kerr POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:kerr>/shaders
    COMMENT "Copying shaders")
