export module ck.tween:motion;

import std;
import :ease;
import :lerp;

// =============================================================================
// Manager-owned, fire-and-forget tweens.
//
//   tw::to(sprite.position, target, 1.0f)
//       .Ease(ck::ease::out_back)
//       .Delay(0.2f)
//       .Tag("intro")
//       .OnDone([]{ /* ... */ });
//
//   auto s = tw::seq()
//       .Then(scale, Vec2{1.2f, 1.2f}, 0.05f)
//       .Then(scale, Vec2{1.0f, 1.0f}, 0.15f).Ease(ease::out_elastic)
//       .With(color, FLASH, 0.05f)        // parallel with previous step
//       .Wait(0.1f)
//       .Call([]{ /* ... */ });
//
//   tw::tick(GetFrameTime());             // once per frame
//   tw::kill_tag("intro");                // batch cancel
//
// `from` is sampled lazily at the moment the motion actually starts (after any
// delay), so chained steps with the same target lerp correctly.
// =============================================================================

namespace ck::tween {

// -----------------------------------------------------------------------------: ITween (internal)
class ITween {
 public:
  virtual ~ITween() = default;
  // Returns true while still running, false when done.
  virtual bool Step(float dt) = 0;

  void SetEase(ease::Fn fn) noexcept { ease_ = fn; }
  void SetDelay(float seconds) noexcept { delay_ = seconds; }
  void SetTag(std::string_view t) { tag_ = t; }
  void SetOnDone(std::function<void()> cb) { on_done_ = std::move(cb); }

  void Cancel() noexcept { canceled_ = true; }
  std::string_view Tag() const noexcept { return tag_; }

 protected:
  ease::Fn ease_ = ease::linear;
  float delay_ = 0.0f;
  bool canceled_ = false;
  std::string tag_;
  std::function<void()> on_done_;

  void FireDone() {
    if (on_done_) on_done_();
  }
};

// -----------------------------------------------------------------------------: Tween<T>
template <Lerpable T>
class Tween : public ITween {
 public:
  Tween(T& target, T value, float duration) noexcept
      : target_(&target), to_(value), duration_(duration) {}

  bool Step(float dt) override {
    if (canceled_) return false;

    if (delay_ > 0.0f) {
      delay_ -= dt;
      if (delay_ > 0.0f) return true;
      dt = -delay_;
      delay_ = 0.0f;
    }

    if (!started_) {
      from_ = *target_;
      started_ = true;
    }

    if (duration_ <= 0.0f) {
      *target_ = to_;
      FireDone();
      return false;
    }

    elapsed_ += dt;
    const float raw = std::clamp(elapsed_ / duration_, 0.0f, 1.0f);
    *target_ = lerp(from_, to_, ease_(raw));

    if (raw >= 1.0f) {
      FireDone();
      return false;
    }
    return true;
  }

 private:
  T* target_;
  T from_{};
  T to_;
  float duration_;
  float elapsed_ = 0.0f;
  bool started_ = false;
};

// -----------------------------------------------------------------------------: utility tweens
class IntervalTween : public ITween {
 public:
  explicit IntervalTween(float seconds) noexcept : remaining_(seconds) {}
  bool Step(float dt) override {
    if (canceled_) return false;
    remaining_ -= dt;
    if (remaining_ <= 0.0f) {
      FireDone();
      return false;
    }
    return true;
  }

 private:
  float remaining_;
};

class CallbackTween : public ITween {
 public:
  explicit CallbackTween(std::function<void()> cb) : cb_(std::move(cb)) {}
  bool Step(float /*dt*/) override {
    if (canceled_) return false;
    if (cb_) cb_();
    FireDone();
    return false;
  }

 private:
  std::function<void()> cb_;
};

// -----------------------------------------------------------------------------: Sequence
class Sequence : public ITween {
  using ParallelStep = std::vector<std::unique_ptr<ITween>>;

 public:
  template <Lerpable T>
  Sequence& Then(T& target, T value, float duration) {
    steps_.emplace_back();
    steps_.back().push_back(
        std::make_unique<Tween<T>>(target, value, duration));
    return *this;
  }

  template <Lerpable T>
  Sequence& With(T& target, T value, float duration) {
    if (steps_.empty()) steps_.emplace_back();
    steps_.back().push_back(
        std::make_unique<Tween<T>>(target, value, duration));
    return *this;
  }

  Sequence& Wait(float seconds) {
    steps_.emplace_back();
    steps_.back().push_back(std::make_unique<IntervalTween>(seconds));
    return *this;
  }

  Sequence& Call(std::function<void()> cb) {
    steps_.emplace_back();
    steps_.back().push_back(std::make_unique<CallbackTween>(std::move(cb)));
    return *this;
  }

  // Apply ease to the most recently added tween. Useful for per-step easing
  // in a fluent chain.
  Sequence& Ease(ease::Fn fn) {
    if (!steps_.empty() && !steps_.back().empty()) {
      steps_.back().back()->SetEase(fn);
    }
    return *this;
  }

  bool Step(float dt) override {
    if (canceled_) return false;
    if (current_ >= steps_.size()) {
      FireDone();
      return false;
    }

    auto& step = steps_[current_];
    bool any_alive = false;
    for (auto& t : step) {
      if (t && t->Step(dt)) {
        any_alive = true;
      } else {
        t.reset();
      }
    }

    if (!any_alive) {
      ++current_;
      // Up to one frame of drift per step transition — acceptable for
      // game-feel sequences; revisit if accurate timing matters.
      if (current_ >= steps_.size()) {
        FireDone();
        return false;
      }
    }
    return true;
  }

 private:
  std::vector<ParallelStep> steps_;
  std::size_t current_ = 0;
};

// -----------------------------------------------------------------------------: Manager (singleton)
class Manager {
 public:
  static Manager& Instance() {
    static Manager m;
    return m;
  }

  template <typename T>
  T* Add(std::unique_ptr<T> t) {
    auto* raw = t.get();
    pending_.push_back(std::move(t));
    return raw;
  }

  void Tick(float dt) {
    // Drain pending into active *before* iterating, so re-entrant adds (e.g.
    // OnDone callbacks creating new tweens) land in the next frame, not this
    // one — keeps the iteration invariant simple.
    for (auto& p : pending_) tweens_.push_back(std::move(p));
    pending_.clear();

    for (auto& t : tweens_) {
      if (t && !t->Step(dt)) t.reset();
    }
    std::erase_if(tweens_, [](auto& p) { return !p; });
  }

  void KillTag(std::string_view tag) {
    const auto match = [tag](auto& p) { return p && p->Tag() == tag; };
    for (auto& t : tweens_)
      if (match(t)) t->Cancel();
    for (auto& t : pending_)
      if (match(t)) t->Cancel();
  }

  void KillAll() noexcept {
    for (auto& t : tweens_)
      if (t) t->Cancel();
    for (auto& t : pending_)
      if (t) t->Cancel();
  }

  std::size_t AliveCount() const noexcept {
    return tweens_.size() + pending_.size();
  }

 private:
  std::vector<std::unique_ptr<ITween>> tweens_;
  std::vector<std::unique_ptr<ITween>> pending_;
};

// -----------------------------------------------------------------------------: TweenHandle (user-facing)
// Lightweight non-owning wrapper. Becomes stale once the underlying tween
// completes (manager destroys it). All mutators are safe on stale handles.
export class TweenHandle {
 public:
  TweenHandle() = default;
  explicit TweenHandle(ITween* p) noexcept : ptr_(p) {}

  TweenHandle& Ease(ease::Fn fn) noexcept {
    if (ptr_) ptr_->SetEase(fn);
    return *this;
  }
  TweenHandle& Delay(float seconds) noexcept {
    if (ptr_) ptr_->SetDelay(seconds);
    return *this;
  }
  TweenHandle& Tag(std::string_view t) {
    if (ptr_) ptr_->SetTag(t);
    return *this;
  }
  TweenHandle& OnDone(std::function<void()> cb) {
    if (ptr_) ptr_->SetOnDone(std::move(cb));
    return *this;
  }
  void Kill() noexcept {
    if (ptr_) ptr_->Cancel();
  }

 private:
  ITween* ptr_ = nullptr;
};

// -----------------------------------------------------------------------------: SequenceHandle (user-facing)
export class SequenceHandle {
 public:
  SequenceHandle() = default;
  explicit SequenceHandle(Sequence* p) noexcept : ptr_(p) {}

  template <Lerpable T>
  SequenceHandle& Then(T& target, T value, float duration) {
    if (ptr_) ptr_->Then(target, value, duration);
    return *this;
  }
  template <Lerpable T>
  SequenceHandle& With(T& target, T value, float duration) {
    if (ptr_) ptr_->With(target, value, duration);
    return *this;
  }
  SequenceHandle& Wait(float seconds) {
    if (ptr_) ptr_->Wait(seconds);
    return *this;
  }
  SequenceHandle& Call(std::function<void()> cb) {
    if (ptr_) ptr_->Call(std::move(cb));
    return *this;
  }
  SequenceHandle& Ease(ease::Fn fn) {
    if (ptr_) ptr_->Ease(fn);
    return *this;
  }
  SequenceHandle& Tag(std::string_view t) {
    if (ptr_) ptr_->SetTag(t);
    return *this;
  }
  SequenceHandle& OnDone(std::function<void()> cb) {
    if (ptr_) ptr_->SetOnDone(std::move(cb));
    return *this;
  }
  void Kill() noexcept {
    if (ptr_) ptr_->Cancel();
  }

 private:
  Sequence* ptr_ = nullptr;
};

// -----------------------------------------------------------------------------: free function API
export template <Lerpable T>
TweenHandle to(T& target, T value, float duration) {
  auto t = std::make_unique<Tween<T>>(target, value, duration);
  auto* raw = Manager::Instance().Add(std::move(t));
  return TweenHandle{raw};
}

export inline SequenceHandle seq() {
  auto s = std::make_unique<Sequence>();
  auto* raw = Manager::Instance().Add(std::move(s));
  return SequenceHandle{raw};
}

export inline void tick(float dt) { Manager::Instance().Tick(dt); }

export inline void kill_tag(std::string_view tag) {
  Manager::Instance().KillTag(tag);
}

export inline void kill_all() noexcept { Manager::Instance().KillAll(); }

export inline std::size_t alive_count() noexcept {
  return Manager::Instance().AliveCount();
}

}  // namespace ck::tween
