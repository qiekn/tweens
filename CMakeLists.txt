cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer libc++ on Clang so modules built here match the parent block project,
# which is libc++ only.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

project(ck_tween VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------: ck_tween module library
# `tween.cppm` is the primary module interface; `ease.cppm` is a partition that
# it re-exports. New partitions (smooth, seq, ...) get added to FILES below.
add_library(ck_tween)
target_sources(ck_tween
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/tween.cppm
      src/ease.cppm)
target_compile_features(ck_tween PUBLIC cxx_std_23)

add_library(ck::tween ALIAS ck_tween)

# ---------------------------------------------------------------------------: examples
# Off by default — examples pull in raylib (via FetchContent if not found), which
# we don't want as a hard build dep. Configure with -DCK_TWEEN_BUILD_EXAMPLES=ON
# to enable.
option(CK_TWEEN_BUILD_EXAMPLES "Build the curve viewer example" OFF)
if(CK_TWEEN_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
