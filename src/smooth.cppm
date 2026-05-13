export module ck.tween:smooth;

import std;
import :lerp;

namespace ck::tween {

// =============================================================================
// Smooth<T> — persistent exponential-decay tracker.
//
// Caller-owned (no manager). Each frame: push a target, call Update(dt). The
// internal value glides toward the target along an exp curve, never
// overshoots, and is frame-rate independent.
//
// half_life is the time (seconds) for the remaining gap to halve. Rough guide:
//   0.05 - 0.15  snappy UI
//   0.15 - 0.30  camera follow (typical)
//   0.30 - 1.00  lazy / cinematic
//   <= 0         instant (snaps to target)
//
// Use case examples: camera target, HP bar drain, slider thumbs, zoom.
// For one-shot effects with a fixed duration use the manager-backed Tween
// API in :seq instead.
// =============================================================================

export template <Lerpable T>
class Smooth {
 public:
  Smooth() = default;
  explicit Smooth(T initial, float half_life = 0.15f) noexcept
      : value_(initial), target_(initial), half_life_(half_life) {}

  void SetTarget(T t) noexcept { target_ = t; }
  void SetHalfLife(float h) noexcept { half_life_ = h; }

  // Skip the lerp, jump to v. Useful for teleports or initial state.
  void Snap(T v) noexcept {
    value_ = v;
    target_ = v;
  }

  const T& Value() const noexcept { return value_; }
  const T& Target() const noexcept { return target_; }
  float HalfLife() const noexcept { return half_life_; }

  void Update(float dt) noexcept { Update(dt, half_life_); }

  void Update(float dt, float half_life) noexcept {
    if (half_life <= 0.0f || dt <= 0.0f) {
      value_ = target_;
      return;
    }
    const float alpha = 1.0f - std::exp2(-dt / half_life);
    value_ = lerp(value_, target_, alpha);
  }

 private:
  T value_{};
  T target_{};
  float half_life_ = 0.15f;
};

}  // namespace ck::tween
