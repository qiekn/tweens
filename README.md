# ck.tween

> 一个为 raylib 设计的 C++23 module tween 库，给 [`block`](../..) 项目用。
>
> 灵感参考：[qaqelol.itch.io/tweens](https://qaqelol.itch.io/tweens)（Godot 内建 tween 教学），但 API 走的是 DOTween 风格 + 一点自己的取舍。

---

## 0. TL;DR — 5 分钟上手

```sh
# 在 block/dev/tweens/ 目录下
cmake -S . -B build -G Ninja -DCK_TWEEN_BUILD_EXAMPLES=ON
cmake --build build

./build/examples/curve_viewer.exe   # 31 条 easing 曲线一字排开
./build/examples/smooth_demo.exe    # 鼠标跟随 + 半衰期切换
./build/examples/motion_demo.exe    # 点击 retarget / 空格 pulse 序列
```

代码里：

```cpp
import ck.tween;

namespace tw = ck::tween;
namespace ease = ck::ease;

// === 持续平滑追踪 ===  零分配，状态属于宿主
struct Camera {
  tw::Smooth<Vector2> follow;
};
cam.follow.SetTarget(player.position);
cam.follow.Update(dt);  // 用 ctor 给的 half_life
camera.target = cam.follow.Value();

// === 一次性效果 === manager 兜底，链式 config
tw::to(sprite.position, Vector2{100, 0}, 1.0f)
    .Ease(ease::out_back)
    .Delay(0.2f)
    .Tag("intro")
    .OnDone([]{ /* ... */ });

// === 序列 / 并行 ===
tw::seq()
    .Then(sprite.scale, Vector2{1.2f, 1.2f}, 0.05f).Ease(ease::out_back)
    .Then(sprite.scale, Vector2{1.0f, 1.0f}, 0.15f).Ease(ease::out_elastic)
    .With(sprite.color, ck::ORANGE, 0.05f)        // 和上一个并行
    .Wait(0.1f)
    .Call([]{ /* hit pause done */ });

// === 每帧 tick 一次（layer 的 OnUpdate 里）===
tw::tick(GetFrameTime());

// === 批量取消 ===
tw::kill_tag("intro");
tw::kill_all();
```

---

## 1. 设计思路（why this shape）

### 1.1 两层 API，是核心抉择

观察：raylib 游戏里 tween 类型的需求其实分两类，硬塞进同一套 API 写起来都难受：

| 需求 | 例子 | 谁拥有状态 | 是否要 manager | 是否有"终点" |
|---|---|---|---|---|
| **持续平滑追踪** | 相机跟随、HP 条丝滑下降、idle bob | 宿主对象（成员） | 不要 | 没有 |
| **一次性效果** | 按钮按下、伤害弹字、屏幕震动、过场 | manager 兜底 | 要 | 有 |

DOTween 在 C# 里两者都塞进 `transform.DOMove(...)` 这套 API，靠 GC 救场；C++ 没 GC，混在一起容易写出悬挂指针。所以我们拆成两层：

- **Tier 1：`Smooth<T>`** — 嵌在宿主结构体里当成员，零分配、零 manager、永远追当前目标。
- **Tier 2：`tw::to() / tw::seq()`** — 全局 manager 兜底，DOTween 那套 fire-and-forget + 链式 config + 序列。

### 1.2 namespace / API 风格

抛弃 Godot 的 `"position:x"` 字符串属性路径（C++ 没运行时反射，写一套迷你反射不值得）。也抛弃 C# 的扩展方法（`transform.DOMove(...)`，C++ 不支持）。落地成：

- 函数式入口：`tw::to(&target_ref, value, duration)`，链式 config `.Ease().Delay().Tag().OnDone()`。
- 类型靠 C++ 模板自动推导，子字段就是 `&sprite.position.x`（`float*`），比 Godot `"position:x"` 还干净。
- 命名：tween/smooth/lerp 在 `ck::tween::`，easing 自成 `ck::ease::`（独立可用，不一定要绑 tween）。

### 1.3 应对悬挂指针的两手

C++ 没 GC，`tw::to(&sprite.x, ...)` 一旦 sprite 死了就 UB。我们的策略：

1. **`Tag` 系统**：tween 可以打标签，宿主销毁前 `tw::kill_tag("...")` 一刀切。
2. **lambda 重载**（待加）：`tw::to(getter, setter, ...)`，由 lambda 自己负责检查目标活着。

---

## 2. 构建 & 工具链

### 2.1 必需

- **CMake ≥ 3.30**（`CMAKE_EXPERIMENTAL_CXX_IMPORT_STD`）
- **Clang 21+ with libc++**（其它编译器不支持，硬性绑定 `import std;`）
- **必须在 `block/dev/tweens/` 路径下**：根 `CMakeLists.txt` 用相对路径 `../../deps/raylib-hpp/` 找 raylib-hpp。standalone clone 现在跑不起来（fatal error 里写了诊断信息）。

### 2.2 命令

```sh
# 只要库（秒级）
cmake -S . -B build -G Ninja
cmake --build build           # 产出 libck_tween.a

# 加 demo（需要 raylib，首次 raylib 编译 ~60s）
cmake -S . -B build -G Ninja -DCK_TWEEN_BUILD_EXAMPLES=ON
cmake --build build
```

### 2.3 Windows 构建坑：clangd 锁 .pcm

clangd（VSCode/Neovim 的 LSP）会把 `.pcm` 文件内存映射，Windows 下文件被锁住，cmake 想覆盖写就失败。报错：

```
error: unable to open output file '...': 'user-mapped section open'
error: unable to open output file '...': 'Permission denied'
```

修复：build 前杀 clangd，编辑器会自动重启它：

```powershell
Get-Process -Name clangd -ErrorAction SilentlyContinue | Stop-Process -Force
cmake --build build
```

详细见项目根 `.claude/skills/clangd-pcm-lock/SKILL.md`（agent 看到那类报错会自动 apply）。

---

## 3. 模块结构

C++23 module，4 个 partition + 1 个聚合入口。

```
src/
├── tween.cppm      # primary interface: export module ck.tween;
├── ease.cppm       # :ease    — 31 个 easing 函数（独立可用）
├── lerp.cppm       # :lerp    — float / Vector2/3/4 / Rectangle / Color overload + Lerpable concept
├── smooth.cppm     # :smooth  — Smooth<T> 模板
└── motion.cppm     # :motion  — Manager + Tween<T> + Sequence + 自由函数 to() / seq() / tick() / kill_tag()
```

`tween.cppm` 只做 `export import :ease/:lerp/:smooth/:motion;`，consumer 一条 `import ck.tween;` 全收。

### 3.1 重要：raylib 类型的取舍

`import raylib;` 把 `::Vector2/3/4/Rectangle` 都暴露成 named entity，但 `::Color` 没有（只暴露 `ck::Color` 包装）。所以 lerp overload 里：

| 类型 | 我们的 overload 签名 |
|---|---|
| 数值 | `template<floating_point T> T lerp(T, T, float)` |
| `::Vector2/3/4`, `::Rectangle` | 直接用 raw POD 类型 |
| Color | `ck::Color lerp(ck::Color, ck::Color, float)` — wrapper |

因为 `ck::Color : public ::Color`，用户传 raw `::Color` 也能 work（derived-to-base 隐式 slicing），写 `ck::Color` 也能 work，两端都不卡。

---

## 4. API 速查

### 4.1 `ck::ease` — 缓动函数（partition `:ease`）

```cpp
namespace ck::ease {
  using Fn = float (*)(float) noexcept;   // 函数指针 typedef

  // 全部 noexcept、float -> float、t in [0,1] -> shaped t in [0,1]
  // back / elastic 微微越界
  inline float linear(float t);

  inline float in_sine(t),    out_sine(t),    in_out_sine(t);
  inline float in_quad(t),    out_quad(t),    in_out_quad(t);
  inline float in_cubic(t),   out_cubic(t),   in_out_cubic(t);
  inline float in_quart(t),   out_quart(t),   in_out_quart(t);
  inline float in_quint(t),   out_quint(t),   in_out_quint(t);
  inline float in_expo(t),    out_expo(t),    in_out_expo(t);
  inline float in_circ(t),    out_circ(t),    in_out_circ(t);
  inline float in_back(t),    out_back(t),    in_out_back(t);     // 越界
  inline float in_elastic(t), out_elastic(t), in_out_elastic(t);  // 越界
  inline float in_bounce(t),  out_bounce(t),  in_out_bounce(t);
}
```

零依赖，不绑 tween。可单独使用：

```cpp
float a = ck::ease::out_cubic(elapsed / 0.3f);
DrawCircle(x, y, 20, tw::lerp(ck::WHITE, ck::RED, a));
```

### 4.2 `ck::tween::lerp` — 插值（partition `:lerp`）

```cpp
namespace ck::tween {
  template <floating_point T>
  T lerp(T a, T b, float t);                       // scalar

  ::Vector2 lerp(::Vector2 a, ::Vector2 b, float t);
  ::Vector3 lerp(::Vector3 a, ::Vector3 b, float t);
  ::Vector4 lerp(::Vector4 a, ::Vector4 b, float t);
  ::Rectangle lerp(::Rectangle, ::Rectangle, float);
  ck::Color lerp(ck::Color, ck::Color, float);     // clamp [0,255]

  template <typename T>
  concept Lerpable = requires(T a, T b, float t) {
    { lerp(a, b, t) } -> std::convertible_to<T>;
  };
}
```

要支持自定义类型：在自己的 namespace 里 overload `lerp(MyType, MyType, float)`，ADL 自动找到。

### 4.3 `ck::tween::Smooth<T>` — 持续平滑（partition `:smooth`）

```cpp
namespace ck::tween {
  template <Lerpable T>
  class Smooth {
   public:
    Smooth();                                              // 默认初始化
    explicit Smooth(T initial, float half_life = 0.15f);

    void SetTarget(T t);
    void SetHalfLife(float seconds);
    void Snap(T v);                                        // 直接跳到 v，不平滑

    const T& Value()    const;
    const T& Target()   const;
    float    HalfLife() const;

    void Update(float dt);                     // 用成员存的 half_life
    void Update(float dt, float half_life);    // 覆盖成员值
  };
}
```

**数学**：指数衰减低通滤波：`alpha = 1 - exp2(-dt / half_life); value = lerp(value, target, alpha);`

**half_life 含义**：剩余差距减半所需的秒数。粗略对照：

| 半衰期 | 感觉 |
|---|---|
| 0.05 ~ 0.15s | 紧凑、UI 用 |
| 0.15 ~ 0.30s | 典型相机跟随 |
| 0.30 ~ 1.00s | 慵懒、电影感 |
| `<= 0` | 立即（snap） |

**特性**：frame-rate independent（60fps / 144fps / 锁 30 同一最终位置），永远不会 overshoot，永远不会到达（asymptotic），无 "结束" 概念。

### 4.4 `ck::tween::*` — 管理器 + 序列（partition `:motion`）

```cpp
namespace ck::tween {
  // === 一次性 tween ===
  template <Lerpable T>
  TweenHandle to(T& target, T value, float duration);

  class TweenHandle {
   public:
    TweenHandle& Ease(ease::Fn fn);
    TweenHandle& Delay(float seconds);
    TweenHandle& Tag(std::string_view t);
    TweenHandle& OnDone(std::function<void()> cb);
    void         Kill();
  };

  // === 序列 ===
  SequenceHandle seq();

  class SequenceHandle {
   public:
    template <Lerpable T>
    SequenceHandle& Then(T& target, T value, float duration);   // 串行
    template <Lerpable T>
    SequenceHandle& With(T& target, T value, float duration);   // 和上一个并行

    SequenceHandle& Wait(float seconds);                         // 空等
    SequenceHandle& Call(std::function<void()> cb);              // 回调步骤
    SequenceHandle& Ease(ease::Fn fn);                           // 改最近一个 tween 的 ease
    SequenceHandle& Tag(std::string_view t);
    SequenceHandle& OnDone(std::function<void()> cb);
    void            Kill();
  };

  // === 全局 ===
  void   tick(float dt);                       // 在 OnUpdate 里调一次
  void   kill_tag(std::string_view tag);       // 取消所有匹配 tag 的
  void   kill_all();
  size_t alive_count();
}
```

**关键设计**：
- `tw::to(...)` 立刻把 tween 注册到 manager 并返回 handle，**不需要 `.Play()`**。
- handle 是 non-owning 包装，tween 结束后 handle 变 stale（所有操作变 no-op，安全）。
- **`from` 是延迟采样的**：tween 真正开始动的那一刻（delay 完之后）才采 `*target_`。这样 sequence 里两个用同一个 target 的 step（`.Then(x,100,1).Then(x,50,1)`）能正确从 100 → 50。
- 序列内部就是 `vector<vector<unique_ptr<ITween>>>` —— 外层串行，内层并行。`Then` 推一个新 step，`With` 追加到当前 step。

---

## 5. 几个常见 pattern

### 5.1 相机跟随玩家

```cpp
struct GameLayer {
  tw::Smooth<Vector2> camera_target{player.position, /*half_life=*/0.20f};

  void OnUpdate(float dt) override {
    camera_target.SetTarget(player.position);
    camera_target.Update(dt);
    camera.target = camera_target.Value();
  }
};
```

### 5.2 HP 条丝滑下降

```cpp
tw::Smooth<float> hp_display{100.0f, 0.30f};

void TakeDamage(int dmg) {
  hp_actual -= dmg;
  hp_display.SetTarget(static_cast<float>(hp_actual));
}

void Render() {
  DrawBar(hp_display.Value() / 100.0f);
}
```

### 5.3 按钮按下反馈

```cpp
void OnClick() {
  tw::seq()
      .Then(button.scale, Vector2{0.85f, 0.85f}, 0.05f).Ease(ease::out_quad)
      .Then(button.scale, Vector2{1.00f, 1.00f}, 0.20f).Ease(ease::out_elastic);
}
```

### 5.4 受击 hit pause + 闪白

```cpp
void OnHit() {
  tw::seq()
      .Then(player.scale, Vector2{1.2f, 1.2f}, 0.04f).Ease(ease::out_back)
      .With(player.color, ck::WHITE,          0.04f)   // 同时变白
      .Then(player.scale, Vector2{1.0f, 1.0f}, 0.20f).Ease(ease::out_elastic)
      .With(player.color, ck::SKYBLUE,        0.25f);  // 同时恢复
}
```

### 5.5 菜单淡入 + 切菜单时 kill 干净

```cpp
void ShowMainMenu() {
  panel.alpha = 0.0f;
  tw::to(panel.alpha, 1.0f, 0.3f).Ease(ease::out_cubic).Tag("main_menu");
}

void GoToSettings() {
  tw::kill_tag("main_menu");      // 防止淡入到一半切场景的悬挂
  // ... build settings menu
}
```

---

## 6. 三个 demo 详解

| Demo | 文件 | 演示什么 |
|---|---|---|
| `curve_viewer` | `examples/curve_viewer.cpp` | 31 条 easing 曲线在 6×6 网格里全画一遍，黄点跟随当前 t —— 用来肉眼对照 back / elastic / bounce 的形状 |
| `smooth_demo` | `examples/smooth_demo.cpp` | 圆圈用 `Smooth<Vector2>` 追鼠标，色调用 `Smooth<ck::Color>` 跟距离插值。LEFT/RIGHT 切 half_life，SPACE 强制 snap |
| `motion_demo` | `examples/motion_demo.cpp` | LMB 用 `tw::to` 把追逐圆 retarget；SPACE 触发盒子的 pulse 序列（shrink + expand + 颜色 flash）；C 用 tag kill 追逐；R kill all + 复位 |

每个 demo 都是 self-contained 单 `.cpp`，可以直接当用法参考。

---

## 7. 已知限制 / footgun

| 项 | 影响 | 何时处理 |
|---|---|---|
| 悬挂指针没有保护 | `tw::to(&sprite.x, ...)`，sprite 死了 UB | 用 `Tag` + `kill_tag` 自己管；以后加 lambda 重载兜底 |
| Sequence step 切换有 ≤ 1 帧 drift | 累计跑长 sequence 会偏几十 ms | 现在够用，将来要精确时序需要 plumb "unused dt" |
| 每个 tween 一次堆分配 | 100/s tween 一般无所谓，10000/s 就有压力 | 真有热点再换 pool |
| 没 ping-pong loop | 没法 `.Loops(-1, Loop::Yoyo)` | TODO |
| 没 OnUpdate 钩子 | 没法订阅每帧值变化 | TODO |
| Lambda getter/setter overload 没做 | 只能传 ref，不能传 property 函数 | 看真有需求再加 |
| 不支持 standalone 构建 | 必须在 `block/dev/tweens/` 路径下 | 等需要再加 submodule 支持 |
| 没有单元测试 | 所有 easing / lerp / smooth / motion 都是手测 | 加 catch2 / doctest |

---

## 8. 开发历程（今天的对话总结）

### 8.1 起点
- 你给了我一个空的 `dev/tweens` repo（init + README 指向 [qaqelol.itch.io/tweens](https://qaqelol.itch.io/tweens)）
- 那个链接其实不是库，是 Godot 内建 tween 的交互式教学
- 你说稍微用过 DOTween，希望我自由设计

### 8.2 设计阶段（没动手前的讨论）
1. 先讲清 tween 数学本质：`value(t) = A + (B - A) * f(t/D)`，f 是 easing
2. 对比三种 API 风格（Godot string property / C++ 指针 / lambda），最后选 C++ 指针风格
3. 抛开 DOTween 思路，提出 **两层 API**：
   - `Smooth<T>` 持续追踪（无 manager，状态归宿主）
   - `tw::to() / tw::seq()` 一次性效果（manager 兜底，DOTween 风格 chain）
4. 这层抽象的依据：DOTween 在 C# 用 GC 兜底悬挂指针；C++ 没 GC，两类需求最好分开管

### 8.3 实现顺序（按依赖关系倒推：3 → 1 → 2）
1. **`:ease` partition**：31 个纯函数 + `Fn` typedef + `curve_viewer` 可视化 demo
2. **`:lerp` partition**：基础插值原语 + `Lerpable` concept
3. **`:smooth` partition**：`Smooth<T>` 模板 + 指数衰减数学 + `smooth_demo`
4. **`:motion` partition**：Manager 单例 + virtual `ITween` + `Tween<T>` + `Sequence` + `TweenHandle/SequenceHandle` + 自由函数 + `motion_demo`

### 8.4 中途的 pivot
- 你指出**不能 `#include <raylib.h>`**，必须走 raylib-hpp 的 `import raylib;`
- 同时指出**应该用 `import std;`**，不要再 `#include <vector>` 等
- 我把所有 `.cppm` 重写了：去掉 `module;` 全局片段和所有 `#include`，换成 `import std;` + `import raylib;`
- 例子改用 `ck::raii::Window` + `ck::Drawing` 等包装
- 关键发现：`import raylib;` 只暴露 `ck::Color`，不暴露 `::Color`。lerp 的 Color overload 用 `ck::Color`，靠 `ck::Color : public ::Color` 的继承让两种用户都能传

### 8.5 又一个 pivot
- 你说也别用 `rl::TextFormat`，应该完全走 raylib-hpp。改成 `std::format(...).c_str()`
- 你的 memory 又加了一条：尽量用 `ck::WHITE`/`ck::SKYBLUE` 等命名常量，少手写 RGBA 字面量。我把例子里能换的都换了

### 8.6 顺手做的 skill
- Windows 上 clangd memory-map `.pcm` 文件导致 cmake build 写文件被拒
- 写了 `block/.claude/skills/clangd-pcm-lock/SKILL.md`，agent 看到这类错误会自动跑 `Get-Process clangd | Stop-Process -Force; cmake --build build`

---

## 9. 下一步候选（按优先级）

1. **Loop / Yoyo**：`.Loops(-1, Loop::Yoyo)` —— `Tween<T>` 内部加方向 flip
2. **OnUpdate 钩子**：每帧 callback，方便订阅值
3. **Lambda getter/setter overload**：`tw::to([&]{ return x; }, [&](T v){ x = v; }, target, dur)` —— 兜底悬挂指针
4. **TweenGroup**：构造时声明一组 tween，析构时统一 kill。比 tag 更 RAII
5. **单元测试**：catch2 或 doctest 接进来，覆盖 ease 边界 / Smooth frame-rate independence / Sequence step 切换
6. **集成进 block**：作为 submodule 拉进 `block/deps/` 试用，跟玩家移动 / 相机 / HP 条对接

---

## 10. 文件清单

```
dev/tweens/
├── CMakeLists.txt
├── README.md                ← 本文档
├── LICENSE
├── .gitignore
├── cmake/
│   └── EnableCxxImportStd.cmake   # 拷自 block，下载 CxxImportStd GUID
├── src/
│   ├── tween.cppm           # primary module interface
│   ├── ease.cppm            # :ease
│   ├── lerp.cppm            # :lerp
│   ├── smooth.cppm          # :smooth
│   └── motion.cppm          # :motion
└── examples/
    ├── CMakeLists.txt
    ├── curve_viewer.cpp
    ├── smooth_demo.cpp
    └── motion_demo.cpp
```

Git 历史（10 commits，每个 commit 一个逻辑变更）：

```
44f381e refactor(example): prefer ck:: named color constants over RGBA literals
7971b28 refactor(example): switch to ck::Color and std::format
61b3461 refactor: migrate to import std and raylib-hpp module
fe1de45 feat(example): add motion demo with chaser and pulse sequence
7ae352a feat(motion): add manager, tween, and sequence API
d12f790 feat(example): add mouse-follow smooth demo
5903fff feat(smooth): add Smooth<T> exponential-decay tracker
004c8c3 feat(lerp): add lerp overloads for raylib types and float
7878ec0 feat(example): add raylib curve viewer for easing functions
6d2cc96 feat(ease): add ck.tween module with 31 easing functions
```
