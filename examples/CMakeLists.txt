# Acquire raylib in the friendliest available way:
#   1. parent already has a `raylib` target → reuse it
#   2. system install findable by CMake     → find_package
#   3. otherwise                            → FetchContent (clones 5.5)

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

add_executable(curve_viewer curve_viewer.cpp)
target_link_libraries(curve_viewer PRIVATE ck::tween raylib)
