<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e297e4f6ce56 -->

# `pluss::ai::nn::GradMode::set_enabled`

[Back to `pluss::ai::nn::GradMode`](../GradMode.md)

## Signature

```cpp
static void nn::GradMode::set_enabled(bool enabled) noexcept;
```

## What It Does

Writes the current thread's grad-recording switch directly. After this
call, [`is_enabled()`](is_enabled.md) returns `enabled` on the calling
thread until something else changes it -- another `set_enabled` call, or an
[`AutoGradMode`](../AutoGradMode.md)/[`NoGradGuard`](../NoGradGuard.md)
guard entering or leaving scope. This is the primitive
[`AutoGradMode`](../AutoGradMode.md) is built from: its constructor reads
the current value, calls `set_enabled(enabled)`, and its destructor calls
`set_enabled(previous)` -- so a bare `set_enabled` call is exactly one half
of that pair, with no restore attached. Prefer a guard over calling this
directly whenever the disabled stretch has a single enclosing scope to
attach one to (nearly always); reach for `set_enabled` itself only when it
does not.

## Parameters

`bool enabled`
: The value to write. `true` allows new operations on slope-tracking
  `Series` values to record a graph node (the default on every thread);
  `false` suppresses recording entirely for every op that runs until the
  switch is changed back.

## Returns

`void`. Mutates the thread-local switch that [`is_enabled()`](is_enabled.md)
reads; there is nothing to check on return.

## Errors

None. `set_enabled` is `noexcept` and has no preconditions -- it accepts
either boolean value unconditionally.

## Practical Examples

Three examples: the direct effect on whether an op's output tracks its
slope, the manual save/restore pattern this call requires when used without
a guard (and the footgun that motivates the guard types), and a realistic
train-vs-evaluate split in a small forward pass.

### Example 1

With grad mode enabled, multiplying a slope-tracking `Series` by itself
produces an output that tracks its slope too; with grad mode disabled, the
identical call produces an output that does not -- `set_enabled` is the
direct lever behind that difference.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series x = pluss::make_series(std::vector<float>{1.0F, 2.0F, 3.0F}, {3}, f32);
  x.tracks_slope_(true);

  nn::GradMode::set_enabled(true);
  const Series y_tracked = x.mul(x);
  std::cout << "grad mode enabled:  y.tracks_slope() = " << std::boolalpha << y_tracked.tracks_slope() << "\n";

  nn::GradMode::set_enabled(false);
  const Series y_untracked = x.mul(x);
  std::cout << "grad mode disabled: y.tracks_slope() = " << y_untracked.tracks_slope() << "\n";

  nn::GradMode::set_enabled(true);

  return 0;
}
```

Output

```text
grad mode enabled:  y.tracks_slope() = true
grad mode disabled: y.tracks_slope() = false
```

### Example 2

The manual save/restore pair that `AutoGradMode` automates internally.
Shown here explicitly to make the footgun concrete: if anything between the
two `set_enabled` calls returned early or threw, the restore call would
never run, leaving grad mode disabled for the rest of the thread. That
exact gap is why the guard types exist.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const bool previous = nn::GradMode::is_enabled();
  nn::GradMode::set_enabled(false);
  std::cout << "disabled for this stretch: " << std::boolalpha << nn::GradMode::is_enabled() << "\n";

  nn::GradMode::set_enabled(previous);
  std::cout << "restored manually: " << nn::GradMode::is_enabled() << "\n";

  return 0;
}
```

Output

```text
disabled for this stretch: false
restored manually: true
```

### Example 3

A `Fusion` layer's `forward` builds a graph back to its weight during an
ordinary (training-style) call. Flipping grad mode off for an
evaluation-style call over the same layer produces an output with no graph
at all -- cheaper, since there is nothing to record and nothing to free
afterward.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  nn::Fusion fc(3, 2, /*bias=*/false, f32);
  const Series x = pluss::make_series(std::vector<float>{1.0F, 2.0F, 3.0F}, {1, 3}, f32);

  const Series train_out = fc.forward(x);
  std::cout << "training pass: output.tracks_slope() = " << std::boolalpha << train_out.tracks_slope() << "\n";

  nn::GradMode::set_enabled(false);
  const Series eval_out = fc.forward(x);
  std::cout << "eval pass:     output.tracks_slope() = " << eval_out.tracks_slope() << "\n";
  nn::GradMode::set_enabled(true);

  return 0;
}
```

Output

```text
training pass: output.tracks_slope() = true
eval pass:     output.tracks_slope() = false
```

## See Also

- [`is_enabled`](is_enabled.md) -- the corresponding read.
- [`AutoGradMode`](../AutoGradMode.md) / [`NoGradGuard`](../NoGradGuard.md)
  -- the RAII guards that call this for you and restore it on scope exit,
  eliminating the manual-restore footgun in Example 2.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_enabled.html">
    <span class="label">Previous</span>
    <span class="title">is_enabled</span>
  </a>
</div>
<!-- apiref:nav:end -->
