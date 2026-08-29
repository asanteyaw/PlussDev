<!-- pluss-hand-authored -->

# Series::tracks_slope_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::tracks_slope_() const -> Series&
Series::tracks_slope_(bool enabled) const -> Series&
```

## What It Does

`tracks_slope_()` mutates whether this `Series` is tracked for reverse-mode
sensitivity.

This is the root opt-in switch for most autograd examples in the `Series`
reference. If you want a leaf to receive stored sensitivity after a later
`reverse()` call, this is the method you turn on before building the forward
computation.

The underscore matters. This is a mutating state-control method, not a
value-returning numeric transform.

## Parameters

**enabled** (`bool`)

Whether tracking should be enabled after the call.

- `true`: mark the series as tracked
- `false`: mark the series as untracked

The zero-argument overload is shorthand for `tracks_slope_(true)`.

## Returns

`Series&`
    The current `Series`, so the mutating call can chain naturally when useful.

## State Effects

Changing this flag affects later reverse-mode behavior directly:

- when enabled, future differentiable computations can accumulate sensitivity
  back onto this series
- when disabled, future reverse passes do not store sensitivity here as a
  tracked leaf

The current implementation also clears any stored sensitivity when this state is
changed. That keeps old sensitivity state from being mistaken for the new
tracking regime.

## Examples

Example 1. The no-argument form is shorthand for turning tracking on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
std::cout << "before=" << (x.tracks_slope() ? "true" : "false") << "\n";
x.tracks_slope_();
std::cout << "after_enable_default=" << (x.tracks_slope() ? "true" : "false") << "\n";
x.tracks_slope_(false);
std::cout << "after_disable=" << (x.tracks_slope() ? "true" : "false") << "\n";
```

### Output

```text
before=false
after_enable_default=true
after_disable=false
```

Example 2. The tracking flag determines whether reverse-mode stores
sensitivity on the leaf.

```cpp
auto untracked = pluss::make_series({1.0, 2.0, 3.0});
auto tracked = pluss::make_series({1.0, 2.0, 3.0});
tracked.tracks_slope_(true);

untracked.mul(2.0).sum().reverse();
tracked.mul(2.0).sum().reverse();

std::cout << "untracked_has_dx=" << (untracked.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "tracked_has_dx=" << (tracked.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "tracked_dx=" << tracked.sensitivity() << "\n";
```

### Output

```text
untracked_has_dx=false
tracked_has_dx=true
tracked_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
```

Example 3. Disabling tracking clears the currently stored sensitivity for that
leaf.

```cpp
auto x = pluss::make_series({3.0, 4.0});
x.tracks_slope_(true);
x.mul(2.0).sum().reverse();

std::cout << "before_disable=" << (x.has_sensitivity() ? "true" : "false") << "\n";
x.tracks_slope_(false);
std::cout << "after_disable=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

### Output

```text
before_disable=true
after_disable=false
```

## Gotchas And Failure Modes

- This method changes tracking state; it is not itself a differentiable
  operation.
- The zero-argument overload means "enable," not "toggle."
- Disabling tracking does more than prevent future storage. It also clears the
  current stored sensitivity state.

## Summary

Use `tracks_slope_()` before the forward pass to mark leaves that should later
receive reverse-mode sensitivity, and use `tracks_slope_(false)` when you want
that leaf to stop participating as a tracked source.

## See Also

- [`Series::tracks_slope`](tracks_slope.md): inspect the current tracking flag.
- [`Series::reverse`](reverse.md): populate sensitivity on tracked leaves.
- [`Series::clear_sensitivity`](clear_sensitivity.md): clear stored
  sensitivity without disabling tracking.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tracks_slope.html">
    <span class="label">Previous</span>
    <span class="title">tracks_slope</span>
  </a>
  <a class="next-link" href="transpose.html">
    <span class="label">Next</span>
    <span class="title">transpose</span>
  </a>
</div>
