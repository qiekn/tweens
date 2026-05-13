cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer libc++ on Clang so modules built here match the parent block project,
# which is libc++ only.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

project(ck_tween VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------: raylib acquisition
# ck.tween:lerp #includes <raylib.h> for Vector2/Color/Rectangle overloads, so
# raylib must be present at module-compile time. Acquisition order:
#   1. parent (or sibling) project already has a `raylib` target → reuse it
#   2. system install findable by CMake                          → find_package
#   3. otherwise                                                 → FetchContent
if(NOT TARGET raylib)
  find_package(raylib QUIET CONFIG)
endif()

if(NOT TARGET raylib AND NOT raylib_FOUND)
  message(STATUS "[ck_tween] fetching raylib 5.5 via FetchContent")
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(raylib)
  # raylib is pure C; don't let CMake try to scan it for C++ modules.
  set_target_properties(raylib PROPERTIES CXX_SCAN_FOR_MODULES OFF)
endif()

# ---------------------------------------------------------------------------: ck_tween module library
# `tween.cppm` is the primary module interface; the others are partitions it
# re-exports. New partitions (smooth, seq, ...) get added to FILES below.
add_library(ck_tween)
target_sources(ck_tween
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/tween.cppm
      src/ease.cppm
      src/lerp.cppm
      src/smooth.cppm
      src/motion.cppm)
target_compile_features(ck_tween PUBLIC cxx_std_23)
# PUBLIC because lerp overloads expose raylib types in their signatures —
# anyone who imports ck.tween needs raylib to compile.
target_link_libraries(ck_tween PUBLIC raylib)

add_library(ck::tween ALIAS ck_tween)

# ---------------------------------------------------------------------------: examples
# Off by default. Configure with -DCK_TWEEN_BUILD_EXAMPLES=ON to enable.
option(CK_TWEEN_BUILD_EXAMPLES "Build the curve viewer example" OFF)
if(CK_TWEEN_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
