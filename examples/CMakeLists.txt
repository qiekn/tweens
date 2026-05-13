# raylib is acquired in the root CMakeLists.txt (ck_tween PUBLIC-links it), so
# each executable inherits the include path and link line transitively.

add_executable(curve_viewer curve_viewer.cpp)
target_link_libraries(curve_viewer PRIVATE ck::tween)

add_executable(smooth_demo smooth_demo.cpp)
target_link_libraries(smooth_demo PRIVATE ck::tween)
