module;

#include <algorithm>
#include <cmath>
#include <concepts>

#include <raylib.h>

export module ck.tween:lerp;

// =============================================================================
// Linear interpolation overloads — the basic building block under every
// `Smooth<T>` and `Tween<T>` in this library. Add new overloads here when the
// game grows a new tweenable type.
//
// Convention: `lerp(a, b, t)` with t in [0, 1] returns a value between `a`
// (at t=0) and `b` (at t=1). Overshoot (t outside [0, 1]) is allowed for
// numeric types — Color clamps because uint8 can't represent overshoot.
// =============================================================================

namespace ck::tween {

// -----------------------------------------------------------------------------: scalar
export template <std::floating_point T>
constexpr T lerp(T a, T b, float t) noexcept {
  return std::lerp(a, b, static_cast<T>(t));
}

// -----------------------------------------------------------------------------: vectors
export inline ::Vector2 lerp(::Vector2 a, ::Vector2 b, float t) noexcept {
  return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t)};
}

export inline ::Vector3 lerp(::Vector3 a, ::Vector3 b, float t) noexcept {
  return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t),
          std::lerp(a.z, b.z, t)};
}

export inline ::Vector4 lerp(::Vector4 a, ::Vector4 b, float t) noexcept {
  return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t),
          std::lerp(a.z, b.z, t), std::lerp(a.w, b.w, t)};
}

// -----------------------------------------------------------------------------: rectangle
export inline ::Rectangle lerp(::Rectangle a, ::Rectangle b, float t) noexcept {
  return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t),
          std::lerp(a.width, b.width, t), std::lerp(a.height, b.height, t)};
}

// -----------------------------------------------------------------------------: color
// uint8 components: lerp in float space, round, clamp to [0, 255].
// Clamp matters when t is outside [0, 1] (e.g. ease::*_back overshoot would
// otherwise wrap around).
export inline ::Color lerp(::Color a, ::Color b, float t) noexcept {
  const auto byte = [](unsigned char ca, unsigned char cb,
                       float tt) -> unsigned char {
    const float v =
        std::lerp(static_cast<float>(ca), static_cast<float>(cb), tt);
    return static_cast<unsigned char>(std::round(std::clamp(v, 0.0f, 255.0f)));
  };
  return {byte(a.r, b.r, t), byte(a.g, b.g, t), byte(a.b, b.b, t),
          byte(a.a, b.a, t)};
}

// -----------------------------------------------------------------------------: concept
// True iff `lerp(a, b, t)` is callable for T. Uses ADL so user-defined types
// with their own `lerp` overload (in their namespace) satisfy it too.
export template <typename T>
concept Lerpable = requires(T a, T b, float t) {
  { lerp(a, b, t) } -> std::convertible_to<T>;
};

}  // namespace ck::tween
