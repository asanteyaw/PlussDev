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

### Output

```text
before=false
after_enable_default=true
after_disable=false
```

Example 3. Tracking state and stored sensitivity are related, but not the same
thing.

```cpp
auto x = pluss::make_series({9.0, 10.0});
x.tracks_slope_(true);
x.mul(4.0).sum().reverse();
x.clear_sensitivity();

std::cout << "tracks=" << (x.tracks_slope() ? "true" : "false") << "\n";
std::cout << "has_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

### Output

```text
tracks=true
has_dx=false
```

So `tracks_slope()` staying `true` does not imply a stored sensitivity is
currently present.

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
  <a class="prev-link" href="tanh.html">
    <span class="label">Previous</span>
    <span class="title">tanh</span>
  </a>
  <a class="next-link" href="tracks_slope_.html">
    <span class="label">Next</span>
    <span class="title">tracks_slope_</span>
  </a>
</div>
