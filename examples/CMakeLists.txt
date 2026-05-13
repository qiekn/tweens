# raylib is acquired in the root CMakeLists.txt (ck_tween PUBLIC-links it), so
# the executable inherits the include path and link line transitively.

add_executable(curve_viewer curve_viewer.cpp)
target_link_libraries(curve_viewer PRIVATE ck::tween)
