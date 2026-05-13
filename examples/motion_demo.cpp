// Motion demo — fire-and-forget tweens, sequences, parallel steps, tag kill.
//
// Build with: cmake -S . -B build -DCK_TWEEN_BUILD_EXAMPLES=ON
// Run with:   ./build/examples/motion_demo
//
// Controls:
//   LMB        retarget the chaser circle (eased move)
//   space      pulse the box (sequence: shrink → expand → settle + color flash)
//   c          kill all "chase" tweens (cancel any in-flight chaser moves)
//   r          kill all tweens and reset state

import std;
import raylib;

import ck.tween;

namespace tw = ck::tween;
namespace ease = ck::ease;
using namespace ck;
using namespace ck::raii;

int main() {
  Window window(960, 600, "ck.tween - motion demo");
  SetTargetFPS(60);

  Vector2 chaser{480, 300};

  Vector2 box_pos{200, 400};
  Vector2 box_scale{1.0f, 1.0f};
  ck::Color box_color = ck::SKYBLUE;

  const ck::Color rest_color = ck::SKYBLUE;
  const ck::Color flash_color = ck::ORANGE;

  const auto pulse_box = [&] {
    tw::seq()
        .Then(box_scale, Vector2{0.85f, 0.85f}, 0.08f)
        .Ease(ease::out_quad)
        .Then(box_scale, Vector2{1.2f, 1.2f}, 0.12f)
        .Ease(ease::out_back)
        .With(box_color, flash_color, 0.12f)
        .Then(box_scale, Vector2{1.0f, 1.0f}, 0.25f)
        .Ease(ease::out_elastic)
        .With(box_color, rest_color, 0.30f)
        .Tag("box_pulse");
  };

  while (!window.ShouldClose()) {
    const float dt = GetFrameTime();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      tw::to(chaser, GetMousePosition(), 0.5f)
          .Ease(ease::out_cubic)
          .Tag("chase");
    }
    if (IsKeyPressed(KEY_SPACE)) pulse_box();
    if (IsKeyPressed(KEY_C)) tw::kill_tag("chase");
    if (IsKeyPressed(KEY_R)) {
      tw::kill_all();
      chaser = {480, 300};
      box_scale = {1.0f, 1.0f};
      box_color = rest_color;
    }

    tw::tick(dt);

    {
      Drawing draw;
      ClearBackground(ck::Color{25, 25, 30, 255});

      DrawCircleV(chaser, 18.0f, ck::SKYBLUE);
      DrawText(
          std::format("chaser: ({:.0f}, {:.0f})", chaser.x, chaser.y).c_str(),
          12, 12, 14, GRAY);

      const float bw = 80.0f * box_scale.x;
      const float bh = 80.0f * box_scale.y;
      DrawRectangleV({box_pos.x - bw * 0.5f, box_pos.y - bh * 0.5f}, {bw, bh},
                     box_color);
      DrawText("SPACE to pulse", static_cast<int>(box_pos.x) - 60,
               static_cast<int>(box_pos.y) + 60, 14, GRAY);

      DrawText(std::format("alive = {}", tw::alive_count()).c_str(), 12, 30,
               14, GRAY);
      DrawText(
          "LMB: chase   SPACE: pulse   C: kill chase   R: kill all + reset",
          12, window.GetScreenHeight() - 26, 14, GRAY);
    }
  }

  return 0;
}
