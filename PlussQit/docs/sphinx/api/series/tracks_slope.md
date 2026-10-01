<!-- pluss-hand-authored -->

# Series::tracks_slope

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::tracks_slope() const -> bool
```

## What It Does

`tracks_slope()` reports whether this `Series` is currently marked to
participate in reverse-mode tracking.

If the answer is `true`, future differentiable computations that use this
series can treat it as a tracked source and later populate sensitivity onto it
when `reverse()` is called downstream.

If the answer is `false`, the series can still be used in forward computation
normally, but reverse-mode will not store sensitivity onto it as a tracked
leaf.

This is a state-inspection method. It does not change tracking behavior by
itself. For mutation, use [`tracks_slope_()`](tracks_slope_.md).

Conceptually, this answers a setup question, not a differentiation question.

- `tracks_slope()` asks: "if reverse-mode reaches this value later, should a
  sensitivity be stored here?"
- [`reverse()`](reverse.md) asks: "given the finished forward result, what
  sensitivities should propagate backward?"
- [`sensitivity()`](sensitivity.md) asks: "what sensitivity is currently stored
  on this series right now?"

Those three stages belong to one workflow, but they are not interchangeable.

## Parameters

None.

## Returns

`bool`
    `true` when the `Series` is configured for reverse-mode tracking,
    otherwise `false`.

## Examples

Example 1. A fresh `Series` is not tracked by default.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
std::cout << "before=" << (x.tracks_slope() ? "true" : "false") << "\n";
```

So ordinary construction gives you a normal numeric `Series`, not a tracked
autodiff leaf.

### Output

```text
before=false
```

Example 2. The no-argument mutator enables tracking, and disabling flips the
state back again.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
std::cout << "before=" << (x.tracks_slope() ? "true" : "false") << "\n";
x.tracks_slope_();
std::cout << "after_enable_default=" << (x.tracks_slope() ? "true" : "false") << "\n";
x.tracks_slope_(false);
std::cout << "after_disable=" << (x.tracks_slope() ? "true" : "false") << "\n";
```

Notice that this is only configuration state. No derivative has been computed
yet, and no stored sensitivity exists merely because the flag became `true`.

### Output

```text
before=false
after_enable_default=true
after_disable=false
```

Example 3. Tracking state and stored sensitivity are related, but not the same
thing.

```cpp
auto x = pluss::make_series({9.0, 10.0}, pluss::cfg::tracks_slope());
x.mul(4.0).sum().reverse();
x.clear();

std::cout << "tracks=" << (x.tracks_slope() ? "true" : "false") << "\n";
std::cout << "has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

So `tracks_slope()` staying `true` does not imply a stored sensitivity is
currently present.

### Output

```text
tracks=true
has_dL_dx=false
```

## Gotchas And Failure Modes

- `tracks_slope()` is configuration state, not proof that any reverse pass ran.
- `tracks_slope() == true` does not imply `has_sensitivity() == true`.
- A series can participate in forward computation normally even when this is
  `false`; the difference shows up when reverse-mode storage is expected.

## Summary

Use `tracks_slope()` to inspect whether a `Series` is currently configured to
receive reverse-mode sensitivity as a tracked value.

## See Also

- [`Series::tracks_slope_`](tracks_slope_.md): mutate the tracking state.
- [`Series::has_sensitivity`](has_sensitivity.md): inspect stored sensitivity
  presence.
- [`Series::reverse`](reverse.md): propagate sensitivity to tracked leaves.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="trace.html">
    <span class="label">Previous</span>
    <span class="title">trace</span>
  </a>
  <a class="next-link" href="tracks_slope_.html">
    <span class="label">Next</span>
    <span class="title">tracks_slope_</span>
  </a>
</div>
