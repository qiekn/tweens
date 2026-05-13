// Smooth demo — a big circle exponentially tracks the mouse cursor.
//
// Build with: cmake -S . -B build -DCK_TWEEN_BUILD_EXAMPLES=ON
// Run with:   ./build/examples/smooth_demo
//
// Controls:
//   ←/→        cycle half-life
//   space      snap to mouse (skip the lerp)
//   r          reset to center

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "raylib.h"

import ck.tween;

namespace tw = ck::tween;

constexpr std::array<float, 7> kHalfLifes = {
    0.03f, 0.08f, 0.15f, 0.30f, 0.60f, 1.00f, 2.00f,
};

int main() {
  InitWindow(960, 600, "ck.tween — smooth demo");
  SetWindowState(FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);

  const Vector2 center{480, 300};
  tw::Smooth<Vector2> pos(center, 0.15f);
  tw::Smooth<Color> tint(Color{120, 200, 255, 255}, 0.20f);

  std::size_t hl_idx = 2;  // 0.15s
  pos.SetHalfLife(kHalfLifes[hl_idx]);

  while (!WindowShouldClose()) {
    const float dt = GetFrameTime();

    const Vector2 mouse = GetMousePosition();
    pos.SetTarget(mouse);

    if (IsKeyPressed(KEY_RIGHT) && hl_idx + 1 < kHalfLifes.size()) {
      pos.SetHalfLife(kHalfLifes[++hl_idx]);
    }
    if (IsKeyPressed(KEY_LEFT) && hl_idx > 0) {
      pos.SetHalfLife(kHalfLifes[--hl_idx]);
    }
    if (IsKeyPressed(KEY_SPACE)) pos.Snap(mouse);
    if (IsKeyPressed(KEY_R)) pos.Snap(center);

    // Tint smoothly tracks distance to target for visual feedback.
    const float dx = mouse.x - pos.Value().x;
    const float dy = mouse.y - pos.Value().y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    const float k = std::clamp(dist / 200.0f, 0.0f, 1.0f);
    const Color target_tint = {
        static_cast<unsigned char>(std::lerp(120.0f, 255.0f, k)),
        static_cast<unsigned char>(std::lerp(200.0f, 100.0f, k)),
        static_cast<unsigned char>(std::lerp(255.0f, 80.0f, k)),
        255,
    };
    tint.SetTarget(target_tint);

    pos.Update(dt);
    tint.Update(dt);

    BeginDrawing();
    ClearBackground(Color{25, 25, 30, 255});

    // Target marker (where the mouse is right now).
    DrawCircleLines(static_cast<int>(mouse.x), static_cast<int>(mouse.y), 8,
                    Color{255, 255, 255, 120});
    DrawLine(static_cast<int>(mouse.x), static_cast<int>(mouse.y),
             static_cast<int>(pos.Value().x), static_cast<int>(pos.Value().y),
             Color{255, 255, 255, 60});

    // Smoothed body.
    DrawCircleV(pos.Value(), 24.0f, tint.Value());

    DrawText(TextFormat("half_life = %.2fs   (idx %zu/%zu)",
                        kHalfLifes[hl_idx], hl_idx + 1, kHalfLifes.size()),
             12, 12, 18, RAYWHITE);
    DrawText("LEFT / RIGHT: cycle half-life   SPACE: snap   R: reset", 12,
             GetScreenHeight() - 26, 14, GRAY);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
