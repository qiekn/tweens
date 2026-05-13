// Curve viewer — plots every easing in ck::ease as a sanity check.
//
// Build with: cmake -S . -B build -DCK_TWEEN_BUILD_EXAMPLES=ON
// Run with:   ./build/examples/curve_viewer
//
// Each cell shows one easing curve (blue line) and an animated yellow dot
// tracking the current t. back/elastic overshoot is intentionally visible —
// extra vertical padding is reserved in each cell.

import std;
import raylib;

import ck.tween;

namespace ease = ck::ease;
using namespace ck;
using namespace ck::raii;

namespace {

struct EasingEntry {
  std::string_view name;
  ease::Fn fn;
};

constexpr std::array<EasingEntry, 31> kEasings = {{
    {"linear", ease::linear},
    {"in_sine", ease::in_sine},
    {"out_sine", ease::out_sine},
    {"in_out_sine", ease::in_out_sine},
    {"in_quad", ease::in_quad},
    {"out_quad", ease::out_quad},
    {"in_out_quad", ease::in_out_quad},
    {"in_cubic", ease::in_cubic},
    {"out_cubic", ease::out_cubic},
    {"in_out_cubic", ease::in_out_cubic},
    {"in_quart", ease::in_quart},
    {"out_quart", ease::out_quart},
    {"in_out_quart", ease::in_out_quart},
    {"in_quint", ease::in_quint},
    {"out_quint", ease::out_quint},
    {"in_out_quint", ease::in_out_quint},
    {"in_expo", ease::in_expo},
    {"out_expo", ease::out_expo},
    {"in_out_expo", ease::in_out_expo},
    {"in_circ", ease::in_circ},
    {"out_circ", ease::out_circ},
    {"in_out_circ", ease::in_out_circ},
    {"in_back", ease::in_back},
    {"out_back", ease::out_back},
    {"in_out_back", ease::in_out_back},
    {"in_elastic", ease::in_elastic},
    {"out_elastic", ease::out_elastic},
    {"in_out_elastic", ease::in_out_elastic},
    {"in_bounce", ease::in_bounce},
    {"out_bounce", ease::out_bounce},
    {"in_out_bounce", ease::in_out_bounce},
}};

}  // namespace

int main() {
  Window window(1280, 800, "ck.tween - curve viewer", FLAG_WINDOW_RESIZABLE);
  SetTargetFPS(60);

  constexpr int kCols = 6;
  constexpr int kRows = 6;
  constexpr float kCycleSec = 2.0f;

  const ck::Color kBg{25, 25, 30, 255};
  const ck::Color kCellFrame{50, 50, 60, 255};
  const ck::Color kAxisMain{70, 70, 80, 255};
  const ck::Color kAxisDim = kAxisMain.Fade(0.4f);
  const ck::Color kCurve = ck::SKYBLUE;

  float elapsed = 0.0f;

  while (!window.ShouldClose()) {
    elapsed += GetFrameTime();
    const float t01 = std::fmod(elapsed, kCycleSec) / kCycleSec;

    {
      Drawing draw;
      ClearBackground(kBg);

      const int win_w = window.GetScreenWidth();
      const int win_h = window.GetScreenHeight();
      const float cell_w = static_cast<float>(win_w) / kCols;
      const float cell_h = static_cast<float>(win_h) / kRows;

      constexpr float pad_x = 12.0f;
      constexpr float pad_label = 22.0f;
      constexpr float pad_overshoot = 18.0f;
      constexpr int samples = 64;

      for (std::size_t i = 0; i < kEasings.size(); ++i) {
        const auto& e = kEasings[i];
        const int col = static_cast<int>(i % kCols);
        const int row = static_cast<int>(i / kCols);
        const float x0 = col * cell_w;
        const float y0 = row * cell_h;

        const float plot_x = x0 + pad_x;
        const float plot_y = y0 + pad_label + pad_overshoot;
        const float plot_w = cell_w - 2 * pad_x;
        const float plot_h = cell_h - pad_label - 2 * pad_overshoot;

        DrawRectangleLines(static_cast<int>(x0 + 1), static_cast<int>(y0 + 1),
                           static_cast<int>(cell_w - 2),
                           static_cast<int>(cell_h - 2), kCellFrame);

        DrawText(e.name.data(), static_cast<int>(x0 + pad_x),
                 static_cast<int>(y0 + 4), 14, RAYWHITE);

        DrawLine(static_cast<int>(plot_x), static_cast<int>(plot_y + plot_h),
                 static_cast<int>(plot_x + plot_w),
                 static_cast<int>(plot_y + plot_h), kAxisMain);
        DrawLine(static_cast<int>(plot_x), static_cast<int>(plot_y),
                 static_cast<int>(plot_x + plot_w), static_cast<int>(plot_y),
                 kAxisDim);

        Vector2 prev{};
        for (int s = 0; s <= samples; ++s) {
          const float tt = static_cast<float>(s) / samples;
          const float vv = e.fn(tt);
          const Vector2 cur{plot_x + tt * plot_w,
                            plot_y + plot_h - vv * plot_h};
          if (s > 0) DrawLineV(prev, cur, kCurve);
          prev = cur;
        }

        const float vv_now = e.fn(t01);
        const Vector2 dot{plot_x + t01 * plot_w,
                          plot_y + plot_h - vv_now * plot_h};
        DrawCircleV(dot, 4.0f, YELLOW);
      }

      DrawText(std::format("t = {:.2f}", t01).c_str(), 8, win_h - 22, 14,
               GRAY);
    }
  }

  return 0;
}
