export module ck.tween:ease;

import std;

// =============================================================================
// Easing functions for tweening.
//
// Domain:  t in [0, 1]
// Range:   roughly [0, 1] (back/elastic overshoot a few percent on purpose)
//
// References:
//   https://easings.net
//   Robert Penner, "Motion, Tweening, and Easing in Flash" (2002)
// =============================================================================

namespace ck::ease {

// -----------------------------------------------------------------------------: type alias
// Common signature for every easing in this module. Use it to pass an easing
// around (e.g. tween config) without templates.
export using Fn = float (*)(float) noexcept;

// -----------------------------------------------------------------------------: constants
// Not exported — implementation detail.
inline constexpr float kPi = std::numbers::pi_v<float>;

inline constexpr float kBackC1 = 1.70158f;
inline constexpr float kBackC2 = kBackC1 * 1.525f;
inline constexpr float kBackC3 = kBackC1 + 1.0f;

inline constexpr float kElasticC4 = (2.0f * kPi) / 3.0f;
inline constexpr float kElasticC5 = (2.0f * kPi) / 4.5f;

inline constexpr float kBounceN1 = 7.5625f;
inline constexpr float kBounceD1 = 2.75f;

// -----------------------------------------------------------------------------: linear
export inline float linear(float t) noexcept { return t; }

// -----------------------------------------------------------------------------: sine
export inline float in_sine(float t) noexcept {
  return 1.0f - std::cos((t * kPi) * 0.5f);
}
export inline float out_sine(float t) noexcept {
  return std::sin((t * kPi) * 0.5f);
}
export inline float in_out_sine(float t) noexcept {
  return -(std::cos(kPi * t) - 1.0f) * 0.5f;
}

// -----------------------------------------------------------------------------: quad
export inline float in_quad(float t) noexcept { return t * t; }
export inline float out_quad(float t) noexcept {
  const float u = 1.0f - t;
  return 1.0f - u * u;
}
export inline float in_out_quad(float t) noexcept {
  if (t < 0.5f) return 2.0f * t * t;
  const float u = 1.0f - t;
  return 1.0f - 2.0f * u * u;
}

// -----------------------------------------------------------------------------: cubic
export inline float in_cubic(float t) noexcept { return t * t * t; }
export inline float out_cubic(float t) noexcept {
  const float u = 1.0f - t;
  return 1.0f - u * u * u;
}
export inline float in_out_cubic(float t) noexcept {
  if (t < 0.5f) return 4.0f * t * t * t;
  const float u = -2.0f * t + 2.0f;
  return 1.0f - (u * u * u) * 0.5f;
}

// -----------------------------------------------------------------------------: quart
export inline float in_quart(float t) noexcept {
  const float t2 = t * t;
  return t2 * t2;
}
export inline float out_quart(float t) noexcept {
  const float u = 1.0f - t;
  const float u2 = u * u;
  return 1.0f - u2 * u2;
}
export inline float in_out_quart(float t) noexcept {
  if (t < 0.5f) {
    const float t2 = t * t;
    return 8.0f * t2 * t2;
  }
  const float u = -2.0f * t + 2.0f;
  const float u2 = u * u;
  return 1.0f - (u2 * u2) * 0.5f;
}

// -----------------------------------------------------------------------------: quint
export inline float in_quint(float t) noexcept {
  const float t2 = t * t;
  return t2 * t2 * t;
}
export inline float out_quint(float t) noexcept {
  const float u = 1.0f - t;
  const float u2 = u * u;
  return 1.0f - u2 * u2 * u;
}
export inline float in_out_quint(float t) noexcept {
  if (t < 0.5f) {
    const float t2 = t * t;
    return 16.0f * t2 * t2 * t;
  }
  const float u = -2.0f * t + 2.0f;
  const float u2 = u * u;
  return 1.0f - (u2 * u2 * u) * 0.5f;
}

// -----------------------------------------------------------------------------: expo
// Endpoints clamped — the raw formula gives 2^-inf at t=0 and 2^0=1 at t=1
// which is fine analytically but ugly for endpoint comparisons.
export inline float in_expo(float t) noexcept {
  return t <= 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
}
export inline float out_expo(float t) noexcept {
  return t >= 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}
export inline float in_out_expo(float t) noexcept {
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;
  if (t < 0.5f) return std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f;
  return (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
}

// -----------------------------------------------------------------------------: circ
export inline float in_circ(float t) noexcept {
  return 1.0f - std::sqrt(1.0f - t * t);
}
export inline float out_circ(float t) noexcept {
  const float u = t - 1.0f;
  return std::sqrt(1.0f - u * u);
}
export inline float in_out_circ(float t) noexcept {
  if (t < 0.5f) {
    const float v = 2.0f * t;
    return (1.0f - std::sqrt(1.0f - v * v)) * 0.5f;
  }
  const float v = -2.0f * t + 2.0f;
  return (std::sqrt(1.0f - v * v) + 1.0f) * 0.5f;
}

// -----------------------------------------------------------------------------: back
// Slight overshoot (~10%) on the leaving/arriving end.
export inline float in_back(float t) noexcept {
  return kBackC3 * t * t * t - kBackC1 * t * t;
}
export inline float out_back(float t) noexcept {
  const float u = t - 1.0f;
  return 1.0f + kBackC3 * u * u * u + kBackC1 * u * u;
}
export inline float in_out_back(float t) noexcept {
  if (t < 0.5f) {
    const float v = 2.0f * t;
    return (v * v * ((kBackC2 + 1.0f) * v - kBackC2)) * 0.5f;
  }
  const float v = 2.0f * t - 2.0f;
  return (v * v * ((kBackC2 + 1.0f) * v + kBackC2) + 2.0f) * 0.5f;
}

// -----------------------------------------------------------------------------: elastic
// Damped sine — springy.
export inline float in_elastic(float t) noexcept {
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;
  return -std::pow(2.0f, 10.0f * t - 10.0f) *
         std::sin((10.0f * t - 10.75f) * kElasticC4);
}
export inline float out_elastic(float t) noexcept {
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;
  return std::pow(2.0f, -10.0f * t) *
             std::sin((10.0f * t - 0.75f) * kElasticC4) +
         1.0f;
}
export inline float in_out_elastic(float t) noexcept {
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;
  if (t < 0.5f) {
    return -(std::pow(2.0f, 20.0f * t - 10.0f) *
             std::sin((20.0f * t - 11.125f) * kElasticC5)) *
           0.5f;
  }
  return (std::pow(2.0f, -20.0f * t + 10.0f) *
          std::sin((20.0f * t - 11.125f) * kElasticC5)) *
             0.5f +
         1.0f;
}

// -----------------------------------------------------------------------------: bounce
// out_bounce is the canonical form; in_bounce and in_out_bounce derive from it.
export inline float out_bounce(float t) noexcept {
  if (t < 1.0f / kBounceD1) {
    return kBounceN1 * t * t;
  }
  if (t < 2.0f / kBounceD1) {
    t -= 1.5f / kBounceD1;
    return kBounceN1 * t * t + 0.75f;
  }
  if (t < 2.5f / kBounceD1) {
    t -= 2.25f / kBounceD1;
    return kBounceN1 * t * t + 0.9375f;
  }
  t -= 2.625f / kBounceD1;
  return kBounceN1 * t * t + 0.984375f;
}
export inline float in_bounce(float t) noexcept {
  return 1.0f - out_bounce(1.0f - t);
}
export inline float in_out_bounce(float t) noexcept {
  return t < 0.5f ? (1.0f - out_bounce(1.0f - 2.0f * t)) * 0.5f
                  : (1.0f + out_bounce(2.0f * t - 1.0f)) * 0.5f;
}

}  // namespace ck::ease
