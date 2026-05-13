cmake_minimum_required(VERSION 3.30)

# Toolchain must be set BEFORE project() so the import-std machinery sees it.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/EnableCxxImportStd.cmake)

project(ck_tween VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------: raylib-hpp acquisition
# Policy: never `#include <raylib.h>` — go through `import raylib;` (raylib-hpp
# module surface). The library and examples use `ck::raii::Window`,
# `ck::Drawing`, `ck::WHITE` etc. exclusively.
#
# Currently dev/tweens expects to be built from inside the parent block project
# at block/dev/tweens/, so it picks up the parent's raylib-hpp + raylib clones.
# Standalone build will require wiring up a submodule path; not done yet.
if(NOT TARGET raylib_hpp_modules)
  set(_raylib_hpp_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../deps/raylib-hpp")
  if(NOT EXISTS "${_raylib_hpp_dir}/CMakeLists.txt")
    message(FATAL_ERROR
      "raylib-hpp not found at ${_raylib_hpp_dir}.\n"
      "dev/tweens currently expects to be built from inside the parent block project. "
      "Build flow: configure from block/ root, or copy this tree into a block-like layout.")
  endif()
  if(NOT EXISTS "${_raylib_hpp_dir}/refs/raylib/CMakeLists.txt")
    message(FATAL_ERROR
      "raylib is missing under ${_raylib_hpp_dir}/refs/raylib (gitignored by raylib-hpp).\n"
      "Run: git clone --depth 1 https://github.com/raysan5/raylib.git ${_raylib_hpp_dir}/refs/raylib")
  endif()
  add_subdirectory("${_raylib_hpp_dir}" "${CMAKE_BINARY_DIR}/_raylib_hpp_build")
endif()

# ---------------------------------------------------------------------------: ck_tween module library
# `tween.cppm` is the primary module interface; the others are partitions it
# re-exports. New partitions get added to FILES below.
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
# PUBLIC: ck.tween:lerp and :motion `import raylib;` and surface raylib types
# in exported signatures, so importers need the raylib_hpp_modules target too.
target_link_libraries(ck_tween PUBLIC raylib_hpp_modules)

add_library(ck::tween ALIAS ck_tween)

# ---------------------------------------------------------------------------: examples
option(CK_TWEEN_BUILD_EXAMPLES "Build the curve viewer and demos" OFF)
if(CK_TWEEN_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
