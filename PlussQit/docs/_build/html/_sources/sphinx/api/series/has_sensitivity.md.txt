<!-- pluss-hand-authored -->

# Series::has_sensitivity

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::has_sensitivity() const -> bool
```

## What It Does

`has_sensitivity()` tells you whether this `Series` currently stores a
reverse-mode sensitivity value.

That wording is deliberate.

The method answers a storage-state question, not a mathematical question. It
does not ask whether the correct sensitivity would be nonzero. It asks whether
a reverse pass has populated a stored sensitivity on this value that has not
since been cleared.

This makes `has_sensitivity()` the right companion to
[`sensitivity()`](sensitivity.md), especially because the current public
surface returns a zero-valued series from `sensitivity()` even before any
reverse pass has populated one.

## Parameters

None.

## Returns

`bool`
    `true` if a stored sensitivity currently exists on this `Series`,
    otherwise `false`.

## Relationship To Nearby Methods

- `tracks_slope()` answers "is this series configured to participate in
  reverse-mode as a tracked value?"
- `has_sensitivity()` answers "does this series currently store a sensitivity
  produced by reverse-mode?"
- `sensitivity()` reads the stored value, or a same-shaped zero series when no
  stored value currently exists

Those are related, but they are not interchangeable.

## Examples

Example 1. A tracked `Series` starts without stored sensitivity, gains it after
reverse, and loses it again after clearing.

```cpp
auto life = pluss::make_series({2.0, 4.0, 8.0});
life.tracks_slope_(true);

std::cout << "before=" << (life.has_sensitivity() ? "true" : "false") << "\n";
life.log().sum().reverse();
std::cout << "after_reverse=" << (life.has_sensitivity() ? "true" : "false") << "\n";
life.clear_sensitivity();
std::cout << "after_clear=" << (life.has_sensitivity() ? "true" : "false") << "\n";
```

### Output

```text
before=false
after_reverse=true
after_clear=false
```

Example 2. If nothing was marked for tracking, reverse-mode does not create a
stored sensitivity on that leaf.

```cpp
auto silent_x = pluss::make_series({1.0, 2.0, 3.0});
silent_x.mul(2.0).sum().reverse();
std::cout << "has_dx=" << (silent_x.has_sensitivity() ? "true" : "false") << "\n";
```

### Output

```text
has_dx=false
```

Example 3. `has_sensitivity()` is the way to distinguish "no stored
sensitivity yet" from "a stored sensitivity that happens to contain zeros."

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

std::cout << "has_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
std::cout << x.sensitivity();
```

### Output

```text
has_dx=false
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

The zero values alone do not prove a reverse pass happened. The boolean state
from `has_sensitivity()` is what disambiguates that case.

## Gotchas And Failure Modes

- `has_sensitivity()` can be `false` even when `tracks_slope()` is `true`.
- A zero-valued `sensitivity()` result does not imply `has_sensitivity()` is
  `true`.
- Clearing sensitivity resets this method back to `false` without disabling
  tracking.

## Summary

Use `has_sensitivity()` as the authoritative test for whether a `Series`
currently stores reverse-mode sensitivity.

## See Also

- [`Series::sensitivity`](sensitivity.md): read the stored value itself.
- [`Series::tracks_slope`](tracks_slope.md): inspect tracking configuration.
- [`Series::clear_sensitivity`](clear_sensitivity.md): clear stored
  sensitivity.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="histc.html">
    <span class="label">Previous</span>
    <span class="title">histc</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
