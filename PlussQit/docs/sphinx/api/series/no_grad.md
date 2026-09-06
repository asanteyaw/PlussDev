<!-- pluss-hand-authored -->

# pluss::NoGradGuard

[Back to Series API Reference](../../series_api_reference)

## Public Types

```cpp
pluss::NoGradGuard guard;
pluss::AutoGradMode mode(bool enabled);
pluss::PauseRecording pause;
```

Include `<pluss/pluss.h>`.

These are scope guards. Construct one at the beginning of a block; its effect
lasts until control leaves that block, including when an exception unwinds the
stack.

## What NoGradGuard Does

`pluss::NoGradGuard` temporarily stops new `Series` operations from joining the
reverse graph. It is intended for work whose derivatives will never be needed:
evaluation, reporting, validation metrics, data-dependent masks, and constants
computed from tracked values.

The distinction is about **operations**, not about changing the input:

- A `Series` that already tracks slope continues to do so.
- A result computed from it while the guard is active does not track slope.
- Once the guard leaves scope, later operations on the original `Series` are
  recorded normally again.

That makes the guard different from [`detach()`](detach.md). `detach()` returns
one graph-independent view. `NoGradGuard` establishes a temporary policy for
every operation in a lexical scope.

## Choosing A Guard

### `pluss::NoGradGuard`

Use this for the ordinary case: recording should be off for an entire block.
It is concise, nestable, and restores the previous state automatically.

### `pluss::AutoGradMode`

Use `pluss::AutoGradMode(false)` when code needs to choose the mode from a
runtime condition. Use `pluss::AutoGradMode(true)` for a smaller nested block
that must record even though an enclosing `NoGradGuard` disabled recording.
On destruction, it restores the mode that was active before construction.

### `pluss::PauseRecording`

`pluss::PauseRecording` also prevents new graph links, but it represents a
temporary suspension rather than a mode choice. It is useful in reusable
library code that must avoid recording its own bookkeeping without deciding
whether the caller is generally in training or evaluation mode. Pauses are
nestable and recording resumes only after the outermost pause ends.

For application code, prefer `NoGradGuard`. Reach for `PauseRecording` when
writing infrastructure that may itself run inside another recording policy.

## Examples

### Evaluation Without Building A Reverse Graph

The input remains a tracked leaf, but the value produced inside the guarded
scope is independent of the reverse graph.

```cpp
auto weights = pluss::make_series({2.0, 3.0, 4.0});
weights.tracks_slope_(true);

pluss::Series prediction;
{
  pluss::NoGradGuard guard;
  prediction = weights.square();
}

std::cout << "weights track: " << weights.tracks_slope() << "\n";
std::cout << "prediction tracks: " << prediction.tracks_slope() << "\n";
pluss::series::print(std::cout, prediction) << "\n";
```

#### Output

```text
weights track: 1
prediction tracks: 0
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4.0000, 9.0000, 16.0000]
```

The numerical calculation still occurs. Only graph construction is disabled.

### Recording Resumes After The Scope

The same leaf can produce an untracked result inside the block and a tracked
result immediately afterward.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

pluss::Series report;
{
  pluss::NoGradGuard guard;
  report = x.mul(10.0);
}

auto objective = x.square().sum();
objective.reverse();

std::cout << "report tracks: " << report.tracks_slope() << "\n";
pluss::series::print(std::cout, x.sensitivity()) << "\n";
```

#### Output

```text
report tracks: 0
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.0000, 4.0000, 6.0000]
```

The report did not join the graph. The later objective did, so its reverse pass
still produces `2 * x`.

### Enabling A Deliberate Inner Calculation

`AutoGradMode(true)` can restore recording for a carefully chosen inner block.

```cpp
auto x = pluss::make_series({2.0, 3.0});
x.tracks_slope_(true);

pluss::Series outside_graph;
pluss::Series inside_graph;
{
  pluss::NoGradGuard guard;
  outside_graph = x.add(1.0);

  {
    pluss::AutoGradMode enable(true);
    inside_graph = x.square();
  }
}

std::cout << "outer result tracks: " << outside_graph.tracks_slope() << "\n";
std::cout << "inner result tracks: " << inside_graph.tracks_slope() << "\n";
```

#### Output

```text
outer result tracks: 0
inner result tracks: 1
```

Keep such overrides narrow. A broad re-enable inside evaluation code can retain
an unexpectedly large graph.

### Suspending Bookkeeping Work

The pause guard is appropriate when a helper computes a diagnostic from a
tracked value but must not alter the caller's surrounding mode.

```cpp
auto x = pluss::make_series({-2.0, 1.0, 4.0});
x.tracks_slope_(true);

pluss::Series positive_mask;
{
  pluss::PauseRecording pause;
  positive_mask = x.gt(0.0);
}

auto loss = x.square().sum();
loss.reverse();

std::cout << "mask tracks: " << positive_mask.tracks_slope() << "\n";
pluss::series::print(std::cout, positive_mask) << "\n";
pluss::series::print(std::cout, x.sensitivity()) << "\n";
```

#### Output

```text
mask tracks: 0
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, true]
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-4.0000, 2.0000, 8.0000]
```

The diagnostic mask remains usable as data. It simply has no reverse history.

## Scope And Thread Rules

The guards are thread-local: disabling recording in one thread does not change
another thread's work. They are also nestable. Destruction restores the prior
state rather than blindly enabling recording, so a helper can safely create a
guard when its caller has already disabled recording.

Store these guards as local variables, not as long-lived global state. Their
lexical lifetime is what makes restoration predictable and exception-safe.

## Gotchas And Failure Modes

- A guard does not erase an existing graph. Values built before the scope keep
  their reverse history.
- `tracks_slope_(true)` inside a guarded block marks a leaf for later work, but
  operations performed before the guard ends are still not recorded.
- An untracked result cannot later recover the operations that produced it.
  Recompute outside the guard if derivatives become necessary.
- In-place mutation still mutates data. Disabling recording is not a rollback,
  copy, or immutability mechanism.
- Keep `AutoGradMode(true)` blocks small so evaluation code does not retain a
  graph accidentally.

## Summary

Use `pluss::NoGradGuard` to perform real `Series` computations without building
new reverse links. Use `pluss::AutoGradMode` when the mode is selected
explicitly, and `pluss::PauseRecording` for nestable infrastructure-level
suspension. All three restore prior state automatically at scope exit.

## See Also

- [`Series::detach`](detach.md): return one graph-independent view.
- [`Series::reverse`](reverse.md): propagate sensitivities through recorded
  operations.
- [`Series::tracks_slope`](tracks_slope.md): inspect whether a value is marked
  for reverse tracking.
- [`Series::tracks_slope_`](tracks_slope_.md): enable or disable tracking on a
  leaf.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nextafter.html">
    <span class="label">Previous</span>
    <span class="title">nextafter</span>
  </a>
  <a class="next-link" href="nonzero.html">
    <span class="label">Next</span>
    <span class="title">nonzero</span>
  </a>
</div>
