<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9ee52db1018c -->

# `pluss::ai::nn::optim::Optimizer::clear`

[Back to `pluss::ai::nn::optim::Optimizer`](../Optimizer.md)

## Signature

```cpp
void nn::optim::Optimizer::clear() const;
```

## What It Does

Resets every parameter this optimizer owns to "no recorded sensitivity",
without touching parameter values. `Series::reverse()` *replaces* a
parameter's sensitivity with whatever this round's backward pass computed
for it -- it does not accumulate across separate `.reverse()` calls the
way some autodiff engines' `.backward()` does. That replacement, though,
only happens for parameters the loss being reversed actually reaches: a
parameter genuinely disconnected from one round's loss graph (a branch
that happened not to use it this round) keeps whatever sensitivity it had
from whenever it was last touched, unless something explicitly resets it.
`clear()` is that reset -- call it before building a new round's loss so a
disconnected parameter correctly reports "no sensitivity this round"
rather than silently reusing a stale value from a previous one. Every
[`step`](step.md) overload built on autodiff (the two closure forms) calls
`clear()` internally for exactly this reason, before invoking the closure.
It also unconditionally re-enables `tracks_slope()` on every owned
parameter, in case something turned it off after construction (see
Example 3) -- an optimizer's parameters are meant to stay trainable for
its whole lifetime, and `clear()` restores that if anything disturbed it.

## Parameters

None.

## Returns

`void`. Mutates every owned parameter's sensitivity state; there is
nothing to check on return.

## Errors

None. `clear` has no preconditions and does not throw.

## Practical Examples

Three examples: confirming `.reverse()` replaces rather than accumulates
sensitivity, the real scenario `clear()` protects against -- a parameter
disconnected from the current round's loss keeping a stale gradient from
an earlier round -- and `clear()` re-enabling `tracks_slope()` on a
parameter that had it turned off.

### Example 1

Two separate `.reverse()` calls on the same parameter, with no `step()` or
`clear()` between them, produce the *same* sensitivity both times (not a
sum) -- confirming replacement, not accumulation. `clear()` then resets it
to "no sensitivity" outright.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series(5.0F, f32);
  w.tracks_slope_(true);
  const Series target = pluss::make_series(1.0F, f32);

  nn::optim::SGD optimizer(std::vector<Series*>{&w}, nn::optim::SGDOptions{.learning_rate = 0.1});

  const Series loss1 = w.sub(target).square().mul(0.5);
  loss1.reverse();
  std::cout << "after 1st reverse(): sensitivity = " << w.sensitivity().item<float>() << "\n";

  const Series loss2 = w.sub(target).square().mul(0.5);
  loss2.reverse();
  std::cout << "after 2nd reverse() (replaced, not summed): sensitivity = " << w.sensitivity().item<float>() << "\n";

  optimizer.clear();
  std::cout << "after clear(): has_sensitivity() = " << std::boolalpha << w.has_sensitivity() << "\n";

  return 0;
}
```

Output

```text
after 1st reverse(): sensitivity = 4
after 2nd reverse() (replaced, not summed): sensitivity = 4
after clear(): has_sensitivity() = false
```

### Example 2

Two parameters, `w1` and `w2`. Round 1's loss touches both. Round 2's loss
touches only `w1` -- without `clear()`, `w2` still reports `w1`'s round-1
sensitivity, even though nothing in round 2 touched `w2` at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w1 = pluss::make_series(5.0F, f32);
  Series w2 = pluss::make_series(2.0F, f32);
  w1.tracks_slope_(true);
  w2.tracks_slope_(true);

  const Series loss_round1 = w1.mul(w1).add(w2.mul(w2));
  loss_round1.reverse();
  std::cout << "round 1: w2.sensitivity() = " << w2.sensitivity().item<float>() << "\n";

  const Series loss_round2 = w1.mul(w1);  // round 2's loss never touches w2
  loss_round2.reverse();
  std::cout << "round 2 (no clear): w2.has_sensitivity() = " << std::boolalpha << w2.has_sensitivity()
             << " -- STALE value " << w2.sensitivity().item<float>() << " left over from round 1\n";

  nn::optim::SGD optimizer(std::vector<Series*>{&w1, &w2});
  optimizer.clear();
  std::cout << "after optimizer.clear(): w2.has_sensitivity() = " << w2.has_sensitivity()
             << " -- stale value discarded\n";

  return 0;
}
```

Output

```text
round 1: w2.sensitivity() = 4
round 2 (no clear): w2.has_sensitivity() = true -- STALE value 4 left over from round 1
after optimizer.clear(): w2.has_sensitivity() = false -- stale value discarded
```

### Example 3

Construction already forces every parameter's `tracks_slope()` to `true`
-- but if something turns it back off afterward, `clear()` forces it back
on again, unconditionally.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  Series w = pluss::make_series(5.0F, f32);
  nn::optim::SGD optimizer(std::vector<Series*>{&w});  // construction already forces tracks_slope_(true)

  w.tracks_slope_(false);  // turned off again, deliberately, after construction
  std::cout << "before clear(): w.tracks_slope() = " << std::boolalpha << w.tracks_slope() << "\n";

  optimizer.clear();
  std::cout << "after clear(): w.tracks_slope() = " << w.tracks_slope() << "\n";

  return 0;
}
```

Output

```text
before clear(): w.tracks_slope() = false
after clear(): w.tracks_slope() = true
```

## See Also

- [`step`](step.md) -- the no-arg overload reads exactly the sensitivity
  state `clear()` resets; the two closure overloads call `clear()`
  internally before running.
- [`StepDiagnosticsOptions`](../StepDiagnosticsOptions.md) --
  `fail_if_no_sensitivity`/`minimum_tracked_coverage` read this same
  per-parameter sensitivity state, so a stale value left by skipping
  `clear()` can mask what those checks are meant to catch.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="last_step_lane_telemetry.html">
    <span class="label">Next</span>
    <span class="title">last_step_lane_telemetry</span>
  </a>
</div>
<!-- apiref:nav:end -->
